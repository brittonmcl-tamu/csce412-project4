#pragma once
#include <string>
/** @file Request.h Data carried by a simulated request. */
/** @brief A request waiting for or receiving service. */
struct Request {
    std::string ipIn; ///< Source IPv4 address.
    std::string ipOut; ///< Destination IPv4 address.
    int processingTime = 0; ///< Required clock cycles.
    char jobType = 'P'; ///< Processing or streaming.
};
