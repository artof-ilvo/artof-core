/**
 * @file Logic.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Additional logic funcionality
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Timing/Clk.h>
#include <memory>

namespace Ilvo {
namespace Utils {
namespace Timing {

    /** @brief Edge detector */
    class EdgeDetector
    {
    private:
        bool previousValue;
    public:
        bool rising;
        bool falling;
    public:
        EdgeDetector();
        ~EdgeDetector() = default;

        void detect(bool newValue);
        bool getValue();
    };

    /** @brief Change detector */
    template <typename T>
    class ChangeDetector
    {
    private:
        T previousValue;
    public:
        bool changed;
    public:
        ChangeDetector() : changed(false) {};
        ~ChangeDetector() = default;

        void detect(T newValue) {
            changed = newValue != previousValue;
            previousValue = newValue;
        };
        T getValue() {
            return previousValue;
        };
    };

    /** @brief Edge detector in time */
    class TimeEdgeDetector
    {
    private:
        Utils::Timing::Clk clk;
        std::chrono::milliseconds detectInterval_ms;
        bool previousValue;
        bool isExpired;
    public:
        TimeEdgeDetector(std::chrono::milliseconds detectInterval_ms);
        ~TimeEdgeDetector() = default;

        bool expired(bool newValue);
    };

    /** @brief Single pulse generator */
    class SinglePulseGenerator
    {
    private:
        Utils::Timing::Clk clk;
        bool timerActivated;
    public:
        SinglePulseGenerator();
        ~SinglePulseGenerator() = default;

        bool generatePulse(std::chrono::milliseconds timerInterval_ms);
    };

    /** @brief Continuous pulse generator */
    class PulseGenerator
    {
    private:
        Utils::Timing::Clk clk;
        std::chrono::milliseconds timerInterval_ms;
        
        bool timerActivated;
        bool p;
    public:
        PulseGenerator(std::chrono::milliseconds timerInterval_ms);
        PulseGenerator(uint timerInterval_ms);
        ~PulseGenerator() = default;

        uint getInterval();
        void setInterval(std::chrono::milliseconds timerInterval_ms);
        void setInterval(uint timerInterval_ms);
        bool generatePulse();

        bool getValue();
    };

} // namespace Ilvo
} // namespace Utils
} // namespace Timing

