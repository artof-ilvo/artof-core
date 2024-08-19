/**
 * @file Gps.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Gps settings loaded from the settings json file
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

static std::map<int, std::string> fixNumber = {
    {0, "Not valid"},
    {1, "RTK Ok"},
    {2, "DGPS"},
    {3, "unavailable"},
    {4, "RTK Fix"},
    {5, "RTK Float"},
};

class Gps : public StateFull
{
public:
    std::string device;
    uint utm_zone;
    uint udp_port;
    std::string ip;
    std::string usb_port;
    std::string ntrip_server;
    std::string ntrip_mountpoint;
    std::string ntrip_uname;
    std::string ntrip_pwd;
    double antenna_rotation;

    Gps();
    Gps(nlohmann::json j);
    ~Gps() = default;

    nlohmann::json prepareJson() const;
};

inline std::ostream& operator<<(std::ostream& os, const Gps& g) {
    os << g.toJson().dump();
    return os;
}

} // namespace
} // namespace
} // namespace