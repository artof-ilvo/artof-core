/**
 * @file Operation.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Operation variable manager.
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#pragma once

#include <memory>

#include <Utils/Timing/Clk.h>
#include <Operation/ImplementControl.h>
#include <Utils/Timing/Logic.h>
#include <Utils/Redis/VariableManager.h>

namespace Ilvo {
namespace Core {

    /**
     * @brief Operation variable manager.
     * 
     * @details The Operation variable manager enpowers the autonomous operation of the robot platform.
     */
    class Operation: public Utils::Redis::VariableManager
    {
    private:       
        // edge detectors
        /** @brief Edge detector for field updates */
        Utils::Timing::EdgeDetector edgeDetectorField;
        /** @brief Edge detector for automatic mode */
        Utils::Timing::EdgeDetector edgeDetectorAutomode;

        // traject
        /** @brief The traject the robot should follow during navigation */
        std::shared_ptr<Utils::Settings::Traject> traject;
        /** @brief Position of the robot in respect to the traject */
        std::shared_ptr<Utils::Settings::PositionData> position;

        /** @brief Set the Redis Json implement states */
        void setRedisJsonImplStates();

        // Controllers
        /** @brief Implement controller */
        ImplementControl implementControl;
    public:
        Operation(const std::string ns);
        ~Operation() = default;

        void init() override;
        void serverTick() override;
    };

} // Core
} // Ilvo

