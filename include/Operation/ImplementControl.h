/**
 * @file ImplementControl.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief 
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#include <Utils/Geometry/Transform.h>
#include <Utils/Timing/Logic.h>
#include <Utils/Timing/Clk.h>
#include <math.h>
#include <Utils/Settings/Field.h>
#include <Utils/Geometry/Point.h>
#include <Utils/Settings/Traject.h>
#include <Utils/Settings/Platform.h>
#include <Utils/Redis/VariableManager.h>
#include <Utils/Settings/Field.h>
#include <map>

namespace Ilvo {
namespace Core {
    
    /** @brief States for discrete implement ImplementControl state machine */
    enum DiscrImplState {DRIVING, SLOW_DOWN, MEASURING};

    /**
     * @brief ImplementControl variable manager
     * 
     * @details The ImplementControl variable manager manages the ImplementControl of the different tasks.
     */
    class ImplementControl
    {
    private:
        /** @brief Variable manager (reference to Navigation) */
        Utils::Redis::VariableManager* manager;
        /** @brief The traject the robot should follow during navigation */
        std::shared_ptr<Utils::Settings::Traject> traject;
        /** @brief Position of the robot in respect to the traject */
        std::shared_ptr<Utils::Settings::PositionData> position;

        bool measuringDiscreteStarted;

        /** @brief FSM state for discrete implement */
        DiscrImplState currentDiscrImplState;
        Utils::Timing::SinglePulseGenerator pulseGenerator;
        /** 
         * @brief Edge detector for busy discrete implement variable 
         * 
         * @details If a falling edge on the busy discrete implement variable is detected, it means that the implement is ready with the measurement.
         * The robot can continue it's ImplementControl to the next point.
        */
        Utils::Timing::EdgeDetector busyDiscrImplEdge;
        /** @brief The robot needs to slow down, appreaching a new discrete implement measurement point. */
        Utils::Timing::EdgeDetector slowDownEdge;
        /** @brief Instructions from the controller to disable the implement ImplementControl, the robot is e.g. in spinning mode */
        bool disableImplement;
    public:
        ImplementControl();
        ~ImplementControl() = default;

        /** @brief Reset the field */
        void reset();
        /** @brief Reset the controller */
        void init(Utils::Redis::VariableManager* manager, std::shared_ptr<Utils::Settings::Traject> traject, std::shared_ptr<Utils::Settings::PositionData> position);
        /** update the ImplementControl */
        void update(bool autoMode);
    private:
        /** @brief Update hitch ImplementControl */
        void updateHitch(Utils::Settings::Task& task);
        /** @brief Update continuous ImplementControl */
        void updateContinuous(Utils::Settings::Task& task);
        /** @brief Update cardan ImplementControl */
        void updateCardan(Utils::Settings::Task& task);
        /** @brief Update discrete ImplementControl */
        void updateDiscrete(Utils::Settings::Task& task);
    };

} // Core
} // Ilvo
