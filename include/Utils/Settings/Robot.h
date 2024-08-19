/**
 * @file Robot.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Robot settings loaded from the settings json file
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <ThirdParty/json.hpp>
#include <Utils/Settings/State.h>

namespace Ilvo {
namespace Utils {
namespace Settings {

    class Robot : public StateFull
    {
    public:
        double width;
        double length;
        double wheel_diameter;

        TransformMatrix tCenter;
        State centerState;

        TransformMatrix tHead;
        State headState;

        Robot();
        Robot(nlohmann::json j);
        ~Robot() = default;

        State& updateCenterState(Eigen::Affine3d);    
        State& updateHeadState(Eigen::Affine3d);    

        Eigen::Affine3d& getCenterTransform();
        State& getCenterState();
        Eigen::Affine3d& getHeadTransform();
        State& getHeadState();

        nlohmann::json prepareJson() const;
    };

    inline std::ostream& operator<<(std::ostream& os, const Robot& d) {
        os << d.toJson().dump();
        return os;
    }

} // namespace
} // namespace
} // namespace
