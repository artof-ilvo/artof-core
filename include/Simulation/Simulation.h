/**
 * @file Simulation.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief 
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Settings/Platform.h>
#include <Utils/Timing/Logic.h>
#include <Utils/Redis/VariableManager.h>
#include <Utils/Timing/Logic.h>
#include <Utils/Settings/Field.h>

namespace Ilvo {
namespace Core {

    /**
     * @brief Simulation variable manager
     * 
     * @details The operation variable manager enables the simulation of the robot platform.
     * Based on the velocity commands the state of the platform is 
     * updated when no robot is attached or hardware-in-the-loop testing is applied on the robot platform.
     */
    class Simulation: public Utils::Redis::VariableManager
    {
    private:
        /** 
         * @brief Edge detector for field updates
         * 
         * @details If a pulse of the field update variable is detected the new field is loaded
         */
        Utils::Timing::EdgeDetector edgeDetectorField;
        std::shared_ptr<Utils::Settings::Field> field;

        /** @brief Flag for when the discrete operation to be active */
        bool discreteImplementActive;
        /** @brief Edge detection on the start discrete implement Redis variable */
        Utils::Timing::EdgeDetector startDiscreteImplementEdge;
        /** @brief Edge detection on the busy discrete implement Redis variable */
        Utils::Timing::EdgeDetector busyDiscrImplEdge;
        /** 
         * @brief Edge detection on the notification acknowledge Redis variable 
         * 
         * @details If a pulse of the notification acknowledge Redis variable is detected, the discrete operation is set as finished.
        */
        Utils::Timing::EdgeDetector notificationAcknowledgeEdge;
        
    public:
        Simulation(const std::string ns);
        ~Simulation() = default;

        void init() override;
        void serverTick() override;
    };

} // Core
} // Ilvo

