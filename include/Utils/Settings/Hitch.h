/**
 * @file Hitch.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Hitch settings loaded from the settings json file
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Settings/State.h>
#include <Utils/String/String.h>


namespace Ilvo {
namespace Utils {
namespace Settings {


class Hitch : public StateFull
{
public:
    uint id;
    std::string name;
    double min;
    double max;
    double link_length;
    std::vector<std::string> types;

    Hitch();
    Hitch(nlohmann::json j);
    ~Hitch() = default;

    Eigen::Affine3d getBallState(double angle);
    std::string getEntityName();

    nlohmann::json prepareJson() const;
    nlohmann::json toStateFullJson(double angle=0.0, int zone=-1);
};

inline std::ostream& operator<<(std::ostream& os, const Hitch& h) {
    os << h.toJson().dump();
    return os;
}

typedef std::shared_ptr<Hitch> HitchPtr;


} // namespace
} // namespace
} // namespace
