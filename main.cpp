/** @file main.cpp Read settings, ask for inputs, and run the simulation. */
#include "LoadBalancer.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

/** @brief Read one integer, rejecting extra text and out-of-range numbers. */
bool readInteger(const std::string& text, int& number) {
    std::istringstream input(text);
    std::string extra;
    return (input >> number) && !(input >> extra);
}

/** @brief Ask for a number; pressing Enter keeps the configuration value. */
bool askForNumber(const std::string& question, int& number) {
    std::cout << question << " [" << number << "]: ";
    std::string answer;
    if (!std::getline(std::cin, answer)) return false;
    if (answer.empty()) return true;
    return readInteger(answer, number);
}

int main() {
    // Simulation settings.
    int num_servers = 20;
    int total_cycles = 100000;
    int update_cycles = 10;
    int request_frequency = 25;
    int batch_min = 1;
    int batch_max = 10;
    int min_task_time = 1;
    int max_task_time = 50;
    int random_seed = 42;
    int color = 0;
    std::string blocked_ips = "none";
    std::string mode = "quiet";
    std::string log_file = "simulation.log";

    std::ifstream configFile("config.txt");
    if (!configFile.is_open()) {
        std::cerr << "Error: Could not open config.txt\n";
        return 1;
    }

    // Read a setting name and value. Allow either KEY=value or KEY value.
    std::string line;
    while (std::getline(configFile, line)) {
        line = line.substr(0, line.find('#'));
        for (char& character : line) {
            if (character == '=') character = ' ';
        }
        std::istringstream input(line);
        std::string key, value, extra;
        if (!(input >> key)) continue;
        if (!(input >> value) || (input >> extra)) {
            std::cerr << "Error: Invalid configuration line: " << line << '\n';
            return 1;
        }
        if (key == "BLOCKED_IPS") blocked_ips = value;
        else if (key == "MODE") mode = value;
        else if (key == "LOG_FILE") log_file = value;
        else {
            int number;
            if (!readInteger(value, number)) {
                std::cerr << "Error: Expected an integer for " << key << '\n';
                return 1;
            }
            if (key == "NUMBER_OF_WEBSERVERS") num_servers = number;
            else if (key == "TOTAL_CYCLES") total_cycles = number;
            else if (key == "CYCLES_BETWEEN_UPDATES") update_cycles = number;
            else if (key == "REQUEST_FREQUENCY") request_frequency = number;
            else if (key == "BATCH_MIN") batch_min = number;
            else if (key == "BATCH_MAX") batch_max = number;
            else if (key == "TASK_MIN") min_task_time = number;
            else if (key == "TASK_MAX") max_task_time = number;
            else if (key == "RANDOM_SEED") random_seed = number;
            else if (key == "COLOR") color = number;
            else {
                std::cerr << "Error: Unknown setting: " << key << '\n';
                return 1;
            }
        }
    }
    if (num_servers < 1 || num_servers > 10000 || total_cycles < 1 || update_cycles < 1 ||
        request_frequency < 0 || request_frequency > 100 || batch_min < 1 ||
        batch_max < batch_min || batch_max > 100000 || min_task_time < 1 ||
        max_task_time < min_task_time || random_seed < 0 || (color != 0 && color != 1) ||
        (mode != "quiet" && mode != "verbose")) {
        std::cerr << "Error: Invalid configuration values\n";
        return 1;
    }

    if (!askForNumber("Starting servers", num_servers) ||
        !askForNumber("Simulation cycles", total_cycles) ||
        num_servers < 1 || num_servers > 10000 || total_cycles < 1) {
        std::cerr << "Error: Enter a positive server count (at most 10000) and duration\n";
        return 1;
    }

    // Comparing actual files also catches symlinks or hard links to config.txt.
    std::error_code pathError;
    if (std::filesystem::equivalent("config.txt", log_file, pathError)) {
        std::cerr << "Error: LOG_FILE must not overwrite the configuration file\n";
        return 1;
    }

    LoadBalancer loadBalancer(num_servers);
    loadBalancer.setTaskTimes(min_task_time, max_task_time);
    loadBalancer.setRandomSeed(random_seed);
    if (!loadBalancer.setBlockedRange(blocked_ips)) {
        std::cerr << "Error: Invalid blocked IP range\n";
        return 1;
    }
    if (!loadBalancer.openLog(log_file, mode == "verbose", color == 1)) {
        std::cerr << "Error: Could not open log file\n";
        return 1;
    }
    if (!loadBalancer.generateRequestQueue(num_servers)) {
        std::cerr << "Error: Too many blocked requests to fill the initial queue\n";
        return 1;
    }
    loadBalancer.printStart(total_cycles);

    std::mt19937 gen(random_seed);
    std::uniform_int_distribution<int> reqDist(1, 100);
    std::uniform_int_distribution<int> batchDist(batch_min, batch_max);

    for (int cycle = 0; cycle < total_cycles; cycle++) {
        loadBalancer.distributeRequests();
        loadBalancer.advanceCycle();
        if (reqDist(gen) <= request_frequency) {
            int new_requests = batchDist(gen);
            for (int i = 0; i < new_requests; i++) {
                loadBalancer.addRequest(loadBalancer.generateRandomRequest());
            }
        }
        if ((cycle + 1) % update_cycles == 0) {
            int current_servers = loadBalancer.getServerCount();
            if (loadBalancer.getRequestQueueSize() < 50ULL * current_servers) {
                loadBalancer.removeWebServer();
            } else if (loadBalancer.getRequestQueueSize() > 80ULL * current_servers) {
                loadBalancer.addWebServer();
            }
        }
    }
    if (!loadBalancer.printSummary()) {
        std::cerr << "Error: Request accounting or writing the log failed\n";
        return 1;
    }
    return 0;
}
