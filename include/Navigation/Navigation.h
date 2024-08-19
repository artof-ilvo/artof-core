/**
 * @file Navigation.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Navigation variable manager.
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#pragma once

#include <memory>

#include <Utils/Timing/Clk.h>
#include <Navigation/NavigationControl.h>
#include <Utils/Timing/Logic.h>
#include <Utils/Redis/VariableManager.h>

namespace Ilvo {
namespace Core {

    /**
     * @brief Navigation variable manager.
     * 
     * @details The Navigation variable manager enpowers the autonomous navigation of the robot platform.
     * It therefore implements a navigation controller that can execute the traject and tasks defined in the field.
     */
    class Navigation: public Utils::Redis::VariableManager
    {
    private:       
        // edge detectors
        /** @brief Edge detector for field updates */
        Utils::Timing::EdgeDetector edgeDetectorField;
        /** @brief Edge detector for automatic mode */
        Utils::Timing::EdgeDetector edgeDetectorAutomode;
        /** @brief Edge detector for algorithm mode */
        Utils::Settings::AlgorithmMode algorithmMode;
        /** @brief Edge detector for algorithm mode */
        Utils::Timing::ChangeDetector<Utils::Settings::AlgorithmMode> changeDetectorAlgorithmMode;
        /** @brief Pulse generator for trajectory end */
        Utils::Timing::SinglePulseGenerator trajectEndReachedPulse;

        // traject
        /** @brief The traject the robot should follow during navigation */
        std::shared_ptr<Utils::Settings::Traject> traject;
        /** @brief Position of the robot in respect to the traject */
        std::shared_ptr<Utils::Settings::PositionData> position;

        void resetPosition();
        void updatePosition();

        // Controllers
        /** @brief Navigation controller */
        NavigationControl navigationControl;

        /** @brief Automatic mode reset (will be processed in the next cycle) */
        bool autoModeReset;
        /** @brief Automatic mode error (will be processed in the next cycle) */
        bool autoModeError;
    public:
        Navigation(const std::string ns);
        ~Navigation() = default;

        void init() override;
        void serverTick() override;
    };

} // Core
} // Ilvo

