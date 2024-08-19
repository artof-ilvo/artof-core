/**
 * @file NavigationControl.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief NavigationControl that implements the navigation algorithm
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#pragma once

#include <memory>
#include <fstream>
#include <vector>

#include <Utils/Settings/Traject.h>
#include <Utils/Redis/VariableManager.h>
#include <Utils/Geometry/Transform.h>
#include <Utils/Settings/Platform.h>
#include <Utils/Pid/PidController.h>
#include <Utils/Timing/Logic.h>

namespace Ilvo {
namespace Core {

    /**
     * @brief NavigationControl that implements the navigation algorithm
     * 
     * @details The controller implements the control algorithm of the robot platform.
     * This control algorithm is a combination of 
     * - a finite state machine (FSM), that depends on platform settings
     * - a navigation algorithm, e.g. pure pursuit.
     */
    class NavigationControl
    {
    private:
        /** @brief Variable manager (reference to Navigation) */
        Utils::Redis::VariableManager* manager;
        /** @brief The traject the robot should follow during navigation */
        std::shared_ptr<Utils::Settings::Traject> traject;
        /** @brief Position of the robot in respect to the traject */
        std::shared_ptr<Utils::Settings::PositionData> position;
        /** @brief Algorithm parameters */
        Utils::Settings::AlgorithmData algorithm;

        /** @brief Pid controller for steady state lateral control (the robot is already close to the path) */
        Utils::Pid::PidController steadyStateLateralController;
        /** @brief Pid controller for rough lateral control (if the lateral position is far from the steady state) */
        Utils::Pid::PidController roughLateralController;

        /** @brief Velocity operation data used during creep operation */
        Utils::Settings::VelocityVector creepVelocity;
        /** @brief Stops the robot's linear operation */
        void stopLinearOperation();
        /** @brief Stops the robot's angular operation */
        bool stopTurning(double currentSmallestAngleDegrees, double earlyStoppingAngle=0.0);

        /** 
         * @brief Spinning for cornering
         * 
         * @details The robot spins for the angle of the corner during cornering. 
         * Note: applicable for 4ws4wd robots or skid steering robots.
         */
        void stateMachineSpinning();
        /** 
         * @brief No spinning for cornering 
         * 
         * @details The robot spins in place 
         */
        void stateMachineNoSpinning();
        /** 
         * @brief G-Turn for cornering
         * 
         * @details The robot spins for 180 degrees when reaching the first corner drives sideways to the second corner and starts driving forward again when reaching the second corner. 
         * Note: applicable for 4ws4wd robots.
         */
        void stateMachineGTurn();
        /**
         * @brief The robot turns and rolls back
         * 
         * @details The robot starts turning when approaching a corner. If the second corner is closer then its turning radius. It rolls back to get in the right postition to take this corner.
         */
        void stateMachineRollBack();

        /**
         * @brief Take the next corner
         * 
         * @param force 
         */
        void nextCorner();

        /** @brief The robot drives using the pure pursuit algorithm */
        void purePursuit();
        /** @brief The robot drives straight line (no algorithm) */
        bool straightLine(double deaccerationDistance=0.5);
        /** @brief The robot spins (rotates in place) */
        bool spinning();
        /** @brief The robot turns (no spinning) */
        bool turn();
        /** @brief The robot drives to a specific corner */
        bool creepToCorner();
        /** @brief The robot drives to a specific position */
        bool creepToPosition();
    public:
        NavigationControl() = default;
        ~NavigationControl() = default;

        /** @brief Reset the field */
        void reset();
        /** @brief Reset the controller */
        void init(Utils::Redis::VariableManager* manager, std::shared_ptr<Utils::Settings::Traject> traject, std::shared_ptr<Utils::Settings::PositionData> position);
        /** @brief Set the velocity operation */
        void setVelocityOperation(double longitudinalVelocity=0.0, double lateralVelocity=0.0, double angularVelocity=0.0);
        /** @brief Set the velocity operation for roll back mode */
        void setTurningVelocity();
        /** @brief Verifies the robot's orientation in respect to the traject */
        void verifyRobotOrientation();
        
        /** 
         * @brief Update the navigation controller 
         * 
         */
        void update(Utils::Settings::AlgorithmMode algorithmMode, bool firstTime=false);

        /**
         * @brief Check if the robot is driving sideways
         * 
         * @return true: the robot is driving sideways
         * @return false: the robot is not driving sideways
         */
        bool getActiveSideways() const;
        const Utils::Settings::AlgorithmData& getAlgorithmData();
    };

    typedef std::shared_ptr<NavigationControl> ControllerPtr;

} // Core
} // Ilvo