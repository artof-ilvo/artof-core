/**
 * @file Platform.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Platform settings loaded from the settings json file
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <ThirdParty/json.hpp>
#include <Utils/Geometry/Point.h>
#include <Utils/Settings/Robot.h>
#include <Utils/Settings/Hitch.h>
#include <Utils/Settings/Gps.h>
#include <string>
#include <memory>

namespace Ilvo {
namespace Utils {
namespace Settings {

    class Velocity
    {    
    public:
        double min;
        double max;

        Velocity() = default;
        Velocity(nlohmann::json j);
        ~Velocity() = default;

        nlohmann::json toJson() const;
    };

    inline std::ostream& operator<<(std::ostream& os, const Velocity& v) {
        os << v.toJson().dump();
        return os;
    }

    enum AlgorithmMode { PP_SPINNING_90 = 1, PP_SPINNING_180 = 2, PURE_PP = 3, PP_ROLL_BACK = 4, EXTERNAL=5 };

    class NavigationMode 
    {
    public:
        AlgorithmMode id;
        std::string name;

        NavigationMode() = default;
        NavigationMode(nlohmann::json j);
        ~NavigationMode() = default;

        nlohmann::json toJson() const;
    };

    inline std::ostream& operator<<(std::ostream& os, const NavigationMode& n) {
        os << n.toJson().dump();
        return os;
    }

    enum AutoModeId { AUTO = 1, STEER = 2, THROTTLE = 3 };

    class AutoMode 
    {
    public:
        AutoModeId id;
        std::string name;

        AutoMode() = default;
        AutoMode(nlohmann::json j);
        ~AutoMode() = default;

        nlohmann::json toJson() const;
    };

    inline std::ostream& operator<<(std::ostream& os, const AutoMode& n) {
        os << n.toJson().dump();
        return os;
    }

    class Platform
    {
    public:
        static Platform& getInstance() {
            static Platform instance_;
            return instance_;
        }

        Platform(Platform const&) = delete;
        void operator=(Platform const&)  = delete;
    private:
        Platform();
        Platform(const std::string& baseFilePath);

        void load(const nlohmann::json& j);
    public:
        std::string name;
        Robot robot;
        Velocity auto_velocity;
        std::vector<NavigationMode> nav_modes;
        std::vector<AutoMode> auto_modes;
        std::vector<Hitch> hitches;
        Gps gps;

        bool navModesContainsId(AlgorithmMode id);
        bool autoModesContainsId(AutoModeId id);
        Hitch& getHitch(std::string name);
        void updateState(Eigen::Affine3d ref);
        Eigen::Affine3d applyVelocityOnRobotRef(Eigen::Affine3d velTransform); 

        nlohmann::json toJson() const;
    };
    inline std::ostream& operator<<(std::ostream& os, const Platform& p) {
        os << p.toJson().dump();
        return os;
    }

} // namespace Ilvo
} // namespace Utils
} // namespace Robot

