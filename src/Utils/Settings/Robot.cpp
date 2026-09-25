#include <Utils/Settings/Robot.h>

using namespace Ilvo::Utils::Settings;
using namespace nlohmann;
using namespace Eigen;


Robot::Robot() : StateFull()
{}

Robot::Robot(json j) : StateFull(j["transform"])
{
    if (j.contains("width")) width = j["width"];
    else width = 1.0;
    if (j.contains("length")) length = j["length"];
    else width = 1.0;
    if (j.contains("wheel_diameter")) wheelDiameter = j["wheel_diameter"];
    else wheelDiameter = 1.0;
    if (j.contains("config")) config = j["config"].get<std::string>();
    else config = "ackermann";
    if (j.contains("transform_center")) tCenter = TransformMatrix(j["transform_center"]);
    else tCenter = tRef;
    if (j.contains("transform_head")) tHead = TransformMatrix(j["transform_head"]);
    else tHead = tRef;
}

json Robot::prepareJson() const {
    json j;
    j["width"] = width;
    j["length"] = length;
    j["wheel_diameter"] = wheelDiameter;
    j["config"] = config;
    j["transform_center"] = tCenter.toJson();
    j["transform_head"] = tHead.toJson();
    return j;
}

State& Robot::updateCenterState(Eigen::Affine3d m)
{
    centerState = State(m);
    return centerState;
}

State& Robot::updateHeadState(Eigen::Affine3d m)
{
    headState = State(m);
    return headState;
}

Affine3d& Robot::getCenterTransform() 
{
    return tCenter;
}

State& Robot::getCenterState()
{
    return centerState;
}

Affine3d& Robot::getHeadTransform() 
{
    return tHead;
}

State& Robot::getHeadState()
{
    return headState;
}
