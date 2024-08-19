/**
 * @file Timing.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Timing constraints validation
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <chrono>
#include <string>

namespace Ilvo {
namespace Utils {
namespace Timing {

    void waitForNetworkConnection(std::string ip, std::chrono::seconds interval);
    void tic(int mode=0);
    void toc();

    double ticHz(int mode=0);
    double tocHz();

    std::string toISO8601Format(std::chrono::system_clock::time_point&);

} // namespace Ilvo
} // namespace Utils
} // namespace Timing