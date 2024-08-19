/**
 * @file Clk.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Timing funcionality of the process
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <chrono>
#include <sys/time.h>
#include <iostream>
#include <thread>


namespace Ilvo {
namespace Utils {
namespace Timing {

    /**
     * @brief manages the timing of a process and keeps track of the update frequency
     * 
     */
    class Clk
    {
    private:
        /** @brief time a new update cycle started */
        std::chrono::system_clock::time_point startTime;
        /** @brief time an additional time started */
        std::chrono::system_clock::time_point timerStartTime;
        /** @brief Flag if the timer is on */
        bool timerOn;
        /** @brief time between two update cycles in milliseconds */
        std::chrono::milliseconds interval;
        /** @brief Additional timer interval */
        std::chrono::milliseconds timerInterval;
    public:
        Clk(std::chrono::milliseconds interval);
        Clk();
        ~Clk() = default;

        /** @brief Start a new update cycle */
        void start();
        /** @brief Poll time of the update cycle */
        double poll();
        /** @brief Stop the update cycle */
        void stop();

        /** @brief Start a timer with a given interval */
        void startTimer(std::chrono::milliseconds timerInterval);
        /** @brief Return true if the timer is busy */
        bool checkTimerBusy();
        /** @brief Return true if the timer has expired */
        bool checkTimerExpired();

        int getIntervalMs();
    };

} // namespace Ilvo
} // namespace Utils
} // namespace Timing
