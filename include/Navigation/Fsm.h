/**
 * @file Fsm.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Base class of the finite state machines (FSM) of the navigation algorithms
 * @version 0.1
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 *
 */

#pragma once

#include <memory>

#include <Utils/Settings/Traject.h>
#include <Utils/Redis/VariableManager.h>
#include <Utils/Geometry/Transform.h>
#include <Utils/Settings/Platform.h>
#include <Utils/Pid/PidController.h>

namespace Ilvo {
namespace Core {

    /**
     * @brief Base class of the finite state machines (FSM) of the navigation algorithms
     *
     * @details A navigation algorithm is a state machine that combines the movements of
     * this class (straight line with pure pursuit, spinning, turning and creeping) to
     * follow the traject and take its corners.
     */
    class Fsm
    {
    protected:
        /** @brief Platform settings */
        Utils::Settings::Platform& platform;
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
        /** @brief Pid controller for pure pursuit) */
        Utils::Pid::PidController purepursuitController;

        /** @brief Velocity operation data used during creep operation */
        Utils::Settings::VelocityVector creepVelocity;
        /** @brief Stops the robot's linear operation */
        void stopLinearOperation();
        /** @brief Stops the robot's angular operation */
        bool stopTurning(double currentSmallestAngleDegrees, double earlyStoppingAngle=0.0);

        /** @brief Go to the STRAIGHTLINE state: reset the lateral controllers and enable the implement again */
        void toStraightLine();
        /** @brief Take the next corner */
        void nextCorner();

        /** @brief The robot drives using the pure pursuit algorithm, at the minimum velocity when slow */
        void purePursuit(bool slow=false);
        /** @brief The robot drives straight line (no algorithm) */
        bool straightLine(double deaccerationDistance=0.5);
        /** @brief The robot spins (rotates in place) */
        bool spinning();
        /** @brief The robot turns (no spinning) */
        bool turn(double earlyStoppingAngle=0.0);
        /**
         * @brief After a turn, the robot drives on with pure pursuit at the minimum velocity until it is straight on the path
         * 
         * @return true while aligning, false when the heading and the position match the path
         */
        bool align();
        /** @brief The robot drives to a specific corner */
        bool creepToCorner();
        /** @brief The robot drives to a specific position */
        bool creepToPosition();
        /** @brief Set the creep velocity for turning with the turning radius */
        void setTurningVelocity();
    public:
        Fsm(Utils::Redis::VariableManager* manager, std::shared_ptr<Utils::Settings::Traject> traject, std::shared_ptr<Utils::Settings::PositionData> position);
        virtual ~Fsm() = default;

        /** @brief Execute one step of the state machine */
        virtual void update() = 0;

        /** @brief Reset the state machine and the controllers */
        void reset();
        /** @brief Set the velocity operation */
        void setVelocityOperation(double longitudinalVelocity=0.0, double lateralVelocity=0.0, double angularVelocity=0.0);

        /**
         * @brief Check if the robot is driving sideways
         *
         * @return true: the robot is driving sideways
         * @return false: the robot is not driving sideways
         */
        bool getActiveSideways() const;
        Utils::Settings::AlgorithmData& getAlgorithmData();
    };

    typedef std::unique_ptr<Fsm> FsmPtr;

} // Core
} // Ilvo
