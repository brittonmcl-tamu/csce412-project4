# Statera Forge: CSCE 412 Project 4

A C++ simulation of a load balancer with a FIFO request queue, dynamically managed web servers,
source-IP filtering, configurable arrivals, and event logging.

## Build and run in WSL

```sh
make
./LB
make test
make docs
make package
```

The program reads config.txt in the directory you run it from. It asks for starting servers
and total cycles; press Enter twice to use the configuration defaults. For an unattended run:

```sh
printf '\n\n' | ./LB
```

MODE in config.txt selects quiet or verbose output. LOG_FILE is overwritten by each run, but it cannot refer to config.txt, including
symlink/hard-link aliases. Save important logs before running again.

## Configuration

Each line is NAME=value or NAME value. Blank lines and # comments are accepted.

| Setting | Meaning |
|---|---|
| NUMBER_OF_WEBSERVERS | Starting server count, 1 through 10000 |
| TOTAL_CYCLES | Positive run duration |
| CYCLES_BETWEEN_UPDATES | Positive interval between scaling checks |
| REQUEST_FREQUENCY | Percentage chance of a new batch each cycle, 0 through 100 |
| BATCH_MIN / BATCH_MAX | Inclusive batch size range, 1 through 100000 |
| TASK_MIN / TASK_MAX | Inclusive positive processing-time range |
| BLOCKED_IPS | Inclusive source-IP range, a single IP, or none/0 |
| RANDOM_SEED | Nonnegative integer seed for repeatable runs |
| MODE | quiet or verbose |
| COLOR | 0 or 1 for verbose terminal event colors |
| LOG_FILE | Output log filename; paths cannot contain spaces |

The default required run starts with 20 servers and 2000 accepted queued requests and runs
100000 cycles. Blocked initial attempts are replaced, with a retry limit to prevent startup
from hanging when nearly every address is blocked. Blocking the entire IPv4 range is rejected.

## How the program works

1. main.cpp reads configuration into ordinary variables, validates it, and asks for inputs.
2. It creates the LoadBalancer and applies task-time, seed, firewall, and log settings.
3. generateRequestQueue() fills the initial queue after the firewall is configured.
4. Each cycle distributes requests, advances servers, and randomly adds new requests.
5. At each update interval, main compares the queue with the current number of servers.
6. After total_cycles, the program prints and saves the final summary.

Newly assigned work processes in that cycle. New arrivals wait for the next cycle. Above
80 requests per server, add one server. Below 50, remove one idle server, keeping at least one.
If every server is busy, defer removal. Removed vector positions can renumber server slots
in the event log. Work still queued or processing when time expires is reported as unfinished.

Quiet mode writes start/end summaries. Verbose mode also writes queue admissions, assignments,
completions, blocks, and scaling actions. Saved logs never contain terminal color codes.
The summary verifies accepted = completed + queued + processing, generated = accepted + blocked
+ invalid, and assigned = completed + processing. Counts include initial requests.

## Files

- Request.h: source IP, destination IP, processing time, and job type.
- WebServer.h/.cpp: request assignment, availability, and the processing countdown.
- LoadBalancer.h/.cpp: queue, workers, random requests, firewall admission, counters, and logging.
- main.cpp: ordinary configuration variables, input, arrivals, and scaling decisions.
- tests/: processing, request preservation, input, firewall, scaling, and repeatability checks.
- docs/index.html: project landing page; docs/design/: branded design and diagram.
- Doxyfile: Doxygen generates docs/generated/html/index.html from the current code comments.

Doxygen generates the API documentation in `docs/generated/html/`.
