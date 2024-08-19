#include <Utils/Settings/Gps.h>
#include <Exceptions/RobotExceptions.hpp>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Exception;

using namespace nlohmann;
using namespace std;

Gps::Gps() : StateFull() {}

Gps::Gps(json j) : StateFull(j["transform"])
{
    if (j.contains("device")) device = j["device"].get<string>();
    else throw SettingsParamNotFoundException("gps", "device");
    if (j.contains("utm_zone")) utm_zone = j["utm_zone"];
    else utm_zone = 31;
    if (j.contains("udp_port")) udp_port = j["udp_port"];
    else udp_port = 0;
    if (j.contains("ip")) ip = j["ip"].get<string>();
    else ip = "";
    if (j.contains("usb_port")) usb_port = j["usb_port"].get<string>();
    else usb_port = "";
    if (j.contains("ntrip_server")) ntrip_server = j["ntrip_server"].get<string>();
    else ntrip_server = "flepos.vlaanderen.be";
    if (j.contains("ntrip_mountpoint")) ntrip_mountpoint = j["ntrip_mountpoint"].get<string>();
    else ntrip_mountpoint = "";
    if (j.contains("ntrip_uname")) ntrip_uname = j["ntrip_uname"].get<string>();
    else ntrip_uname = "";
    if (j.contains("ntrip_pwd")) ntrip_pwd = j["ntrip_pwd"].get<string>();
    else ntrip_pwd = "";
    if (j.contains("antenna_rotation")) antenna_rotation = j["antenna_rotation"];
    else antenna_rotation = 0.0;
}

json Gps::prepareJson() const {
    json j;
    j["device"] = device;
    j["utm_zone"] = utm_zone;
    if (udp_port != 0) j["udp_port"] = udp_port;
    if (!ip.empty()) j["ip"] = ip;
    if (!usb_port.empty()) j["usb_port"] = usb_port;
    j["ntrip_server"] = ntrip_server;
    j["ntrip_mountpoint"] = ntrip_mountpoint;
    j["ntrip_uname"] = ntrip_uname;
    j["ntrip_pwd"] = ntrip_pwd;
    j["antenna_rotation"] = antenna_rotation;
    return j;
}
