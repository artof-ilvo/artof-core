/**
 * @file NavigationUtils.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Util structs and enums for navigation.
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <ThirdParty/json.hpp>
#include <ThirdParty/UTM.hpp>
#include <Utils/Geometry/Point.h>

namespace Ilvo {
namespace Utils {
namespace Settings {

    const std::vector<std::string> continuousOperationTypes = {"continuous", "cardan", "intermittent"};
    const std::vector<std::string> implementTypes = {"continuous", "cardan", "intermittent", "discrete"};

    const double RESET_PATH_DISTANCE_DEFAULT = 2.0;
    const double RESET_PATH_DISTANCE_LARGE = 10.0;

    /** @brief FSM states. */
    enum FsmState {
        STRAIGHTLINE, 
        SPINNING, 
        TURN, 
        CREEP_SIDEWAYS, 
        CREEP_TO_CORNER, 
        CREEP_TO_POSITION
    };
    inline std::map<FsmState, std::string> fsmStateToString = {
        {STRAIGHTLINE, "STRAIGHTLINE"},
        {SPINNING, "SPINNING"},
        {TURN, "TURN"},
        {CREEP_SIDEWAYS, "CREEP_SIDEWAYS"},
        {CREEP_TO_CORNER, "CREEP_TO_CORNER"},
        {CREEP_TO_POSITION, "CREEP_TO_POSITION"}
    };
    /** @brief Line following algorithm. */
    enum LineFollowingMode {PUREPURSUIT, MPC};
    /** @brief Algorithm modes that determine used FSM. */
    enum InterpolationType
    {
        LINEAR,
        CURVY,
        CURRENT
    };
    inline std::map<AlgorithmMode, InterpolationType> algorithmModeToInterpolationType = {
        {PP_SPINNING_90, LINEAR},
        {PP_SPINNING_180, LINEAR},
        {PURE_PP, CURVY},
        {PP_ROLL_BACK, LINEAR}
    };

    class VelocityVector {
        public:
            VelocityVector() = default;
            ~VelocityVector() = default;

            double longitudinal = 0.0;
            double lateral = 0.0;
            double angular = 0.0;

            inline void set(double longitudinal=0.0, double lateral=0.0, double angular=0.0) {
                this->longitudinal = longitudinal;
                this->lateral = lateral;
                this->angular = angular;
            };

            inline nlohmann::json toJson() const {
                nlohmann::json j;
                j["longitudinal"] = longitudinal;
                j["lateral"] = lateral;
                j["angular"] = angular;
                return j;
            }
    };

    /** @brief Pure pursuit data that is used in the state machines. */
    class AlgorithmData {
        public:

            AlgorithmData() = default;
            ~AlgorithmData() = default;

            // Finite state machine
            /** @brief Finite state machine - Current state of the FSM. */
            FsmState fsmState;

            // Velocity
            /** @brief Velocity - Longitudinal velocity during normal operation (outside a task). */
            VelocityVector velocity;
            /** @brief Velocity - Longitudinal task velocity during operation (in a task). */
            double longitudinalTaskVelocity = 0.0;

            // Turning
            /** @brief Turning - Angle (in degrees) to the heading goal. */
            double angleToGoal = 0.0;
            /** @brief Turning - The goal of the heading (in degrees) the robot should turn stop the TURNING state. */
            double headingGoal;  // degrees
            /** @brief Turning - Flag if the robot should turn 180 degrees at the headlands */
            bool turn180 = false;

            // Pure Pursuit
            /** @brief Pure Pursuit - Flag respresents if the navigation controller is in steady state, if yes, sometimes other controllers are used. */
            bool steadyState = false;
            /** @brief Alpha (in degrees), used for calculations in the PurePursuit algorithm. */
            double alpha = 0.0;  // degrees


            inline nlohmann::json toJson() const {
                nlohmann::json j;
                j["fsmState"] = fsmStateToString[fsmState];
                j["velocity"] = velocity.toJson();
                j["longitudinalTaskVelocity"] = longitudinalTaskVelocity;
                j["angleToGoal"] = angleToGoal;
                j["headingGoal"] = headingGoal;
                j["turn180"] = turn180;
                j["steadyState"] = steadyState;
                j["alpha"] = alpha;
                return j;
            }
    };

    class PositionData {
        public:
            PositionData() = default;
            ~PositionData() = default;

            /** @brief The state of the robot (position and orientation) */
            Utils::Settings::State robotRefState;
            /** @brief The state of the robot (position and orientation) */
            Utils::Settings::State robotHeadState;
            /** @brief Turning - Current heading of the robot (in degrees). */
            double heading = 0.0; // degrees
            /** @brief General - Current point. */
            Utils::Geometry::Point currentPoint;
            /** @brief General - Closest point. */
            Utils::Geometry::IndexPoint closestPoint;
            /** @brief Pure Pursuit - Closest carrot point. */
            Utils::Geometry::IndexPoint carrotPoint;
            /** @brief General - Current point. */
            Utils::Geometry::Point headCurrentPoint;
            /** @brief General - Closest point. */
            Utils::Geometry::IndexPoint headClosestPoint;
            /** @brief CornerPoint updating - Next and previous corners, necessary to keep track of the headlands and the turning of the FSM. */
            Utils::Geometry::NextAndPreviousCorner corners;
            /** @brief General - when distance is larger than this value, the closest point is searched over the entire path */
            double resetPathDistance = RESET_PATH_DISTANCE_DEFAULT;

            inline nlohmann::json toJson(int zone=-1) const {
                nlohmann::json j;
                double lat, lng;

                j["heading"] = heading;
                
                j["headCurrent"] = nlohmann::json();
                j["headClosest"] = nlohmann::json();
                j["current"] = nlohmann::json();
                j["closest"] = nlohmann::json();
                j["carrot"] = nlohmann::json();

                j["headCurrent"]["xy"] = {headCurrentPoint.x(), headCurrentPoint.y()};
                j["headClosest"]["xy"] = {headClosestPoint.x(), headClosestPoint.y()};
                j["current"]["xy"] = {currentPoint.x(), currentPoint.y()};
                j["closest"]["xy"] = {closestPoint.x(), closestPoint.y()};
                j["carrot"]["xy"] = {carrotPoint.x(), carrotPoint.y()};

                if ((1 <= zone) && (zone <= 60)) {
                    Geometry::UTMXYToLatLon(headCurrentPoint.x(), headCurrentPoint.y(), zone, false, lat, lng);
                    j["headCurrent"]["latlng"] = {Ilvo::Utils::Geometry::RadToDeg(lat), Ilvo::Utils::Geometry::RadToDeg(lng)};
                    Geometry::UTMXYToLatLon(headClosestPoint.x(), headClosestPoint.y(), zone, false, lat, lng);
                    j["headClosest"]["latlng"] = {Ilvo::Utils::Geometry::RadToDeg(lat), Ilvo::Utils::Geometry::RadToDeg(lng)};
                    Geometry::UTMXYToLatLon(currentPoint.x(), currentPoint.y(), zone, false, lat, lng);
                    j["current"]["latlng"] = {Ilvo::Utils::Geometry::RadToDeg(lat), Ilvo::Utils::Geometry::RadToDeg(lng)};
                    Geometry::UTMXYToLatLon(closestPoint.x(), closestPoint.y(), zone, false, lat, lng);
                    j["closest"]["latlng"] = {Ilvo::Utils::Geometry::RadToDeg(lat), Ilvo::Utils::Geometry::RadToDeg(lng)};
                    Geometry::UTMXYToLatLon(carrotPoint.x(), carrotPoint.y(), zone, false, lat, lng);
                    j["carrot"]["latlng"] = {Ilvo::Utils::Geometry::RadToDeg(lat), Ilvo::Utils::Geometry::RadToDeg(lng)};
                }
                return j;
            }
    };

}
}
}