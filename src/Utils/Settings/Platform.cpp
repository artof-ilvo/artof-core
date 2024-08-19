#include <Utils/Settings/Platform.h>
#include <Utils/Logging/LoggerStream.h>
#include <ThirdParty/json.hpp>
#include <boost/filesystem.hpp>
#include <Exceptions/FileExceptions.hpp>
#include <Exceptions/RobotExceptions.hpp>
#include <ThirdParty/UTM.hpp>
#include <iostream>
#include <string> 

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;

using namespace std;
using namespace nlohmann;
using namespace boost::filesystem;
using namespace Eigen;

// throws error 'std::logic_error'  what():  basic_string::_M_construct null not valid
Platform::Platform() : Platform(string(getenv("ILVO_PATH")))
{
}

Platform::Platform(const string& baseFilePath)
{
    string settings_path = baseFilePath + "/settings.json";
    if ( !exists(settings_path) ) {
        throw PathNotFoundException(settings_path);
    }
    std::ifstream ifs(settings_path);
    json jf;
    try {
        jf = json::parse(ifs);
    } catch(json::exception& e) {
        LoggerStream::getInstance() << ERROR << "Settings file parse error, " << e.what();
        throw runtime_error("Settings file parse error, " + std::string(e.what()));
    }

    load(jf);
    LoggerStream::getInstance() << INFO << "Platform is loaded from: " << settings_path;
}

bool Platform::navModesContainsId(AlgorithmMode id)
{
    for (int i=0; i < nav_modes.size(); i++) {
        NavigationMode mode = nav_modes.at(i);
        if (mode.id == id) return true;
    }
    return false;
}

bool Platform::autoModesContainsId(AutoModeId id)
{
    for (int i=0; i < auto_modes.size(); i++) {
        AutoMode mode = auto_modes.at(i);
        if (mode.id == id) return true;
    }
    return false;
}

Hitch& Platform::getHitch(string name)
{
    if (hitches.size() > 0) {
        for (Hitch& hitch: hitches) {  
            if (hitch.name == name) {
                LoggerStream::getInstance() << DEBUG << "Hitch " << name << " is found back in the robot configuration (settings.json).";
                return hitch;
            }
        }

        LoggerStream::getInstance() << DEBUG << "Hitch " << name << " was not found back in the robot configuration (settings.json), defaulted to " << hitches[hitches.size() - 1].name << ".";
        return (hitches[hitches.size() - 1]);   // return last hitch from line
    } 

    throw HitchNotFoundException(name);
}

void Platform::load(const json& j)
{
    try {
        LoggerStream::getInstance() << INFO << "Loading platform_settings.json";
        name = j["name"].get<string>();

        robot = Robot(j["robot"]);
        auto_velocity = Velocity(j["auto_velocity"]);

        for (json nav_mode: j["nav_modes"]) {
            nav_modes.push_back(NavigationMode(nav_mode));
        }
        for (json auto_mode: j["auto_modes"]) {
            auto_modes.push_back(AutoMode(auto_mode));
        }

        for (json hitch: j["hitches"]) {
            hitches.push_back(Hitch(hitch));
        }

        gps = Gps(j["gps"]);
    } catch(const exception& e) {
        LoggerStream::getInstance() << WARN << "settings.json file is mallformed: " << e.what();
    }

}

void Platform::updateState(Affine3d raw)
{
    gps.updateState(raw * gps.getRefTransform());
    robot.updateState(gps.getState().asAffine() * robot.getRefTransform());
    robot.updateCenterState(gps.getState().asAffine() * robot.getCenterTransform());
    robot.updateHeadState(gps.getState().asAffine() * robot.getHeadTransform()); 

    // update to hitchRef
    for (Hitch& hitch: hitches) {
        hitch.updateState(gps.getState().asAffine() * hitch.getRefTransform());
    }
}

Affine3d Platform::applyVelocityOnRobotRef(Affine3d velTransform) 
{
    robot.updateState(robot.getState().asAffine() * velTransform);
    gps.updateState(robot.getState().asAffine() * robot.getRefTransform().inverse());

    // update robot center state forward
    robot.updateCenterState(gps.getState().asAffine() * robot.getCenterTransform()); 
    robot.updateHeadState(gps.getState().asAffine() * robot.getHeadTransform()); 
    // update to hitchRef, use forwards transformation on gps again
    for (Hitch& hitch: hitches) {
        hitch.updateState(gps.getState().asAffine() * hitch.getRefTransform());
    }

    // return raw state
    return gps.getState().asAffine() * gps.getRefTransform().inverse();
}

json Platform::toJson() const {
    json j;
    j["name"] = name;
    j["robot"] = robot.toJson();
    j["auto_velocity"] = auto_velocity.toJson();
    j["nav_modes"] = json::array();
    for (NavigationMode nav_mode: nav_modes) {
        j["nav_modes"].push_back(nav_mode.toJson());
    }
    j["auto_modes"] = json::array();
    for (AutoMode auto_mode: auto_modes) {
        j["auto_modes"].push_back(auto_mode.toJson());
    }
    j["hitches"] = json::array();
    for (Hitch hitch: hitches) {
        j["hitches"].push_back(hitch.toJson());
    }
    j["gps"] = gps.toJson();
    return j;
}

AutoMode::AutoMode(json j)
{
    if (j.contains("id")) id = j["id"];
    else throw SettingsParamNotFoundException("auto_mode", "id");
    if (j.contains("name")) name = j["name"].get<string>();
    else throw SettingsParamNotFoundException("auto_mode", "name");  
}

NavigationMode::NavigationMode(json j)
{
    if (j.contains("id")) id = j["id"];
    else throw SettingsParamNotFoundException("navigation_mode", "id");
    if (j.contains("name")) name = j["name"].get<string>();
    else throw SettingsParamNotFoundException("navigation_mode", "name");  
}

Velocity::Velocity(json j)
{
    if (j.contains("min")) min = j["min"];
    else throw SettingsParamNotFoundException("velocity", "min");
    if (j.contains("max")) max = j["max"];
    else throw SettingsParamNotFoundException("velocity", "max");  
}

json NavigationMode::toJson() const {
    json j;
    j["id"] = id;
    j["name"] = name;
    return j;
}

json AutoMode::toJson() const {
    json j;
    j["id"] = id;
    j["name"] = name;
    return j;
}

json Velocity::toJson() const {
    json j;
    j["min"] = min;
    j["max"] = max;
    return j;
}
