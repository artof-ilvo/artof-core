#include <Utils/Settings/Platform.h>
#include <Utils/Geometry/Polygon.h>
#include <Utils/Logging/LoggerStream.h>
#include <ThirdParty/json.hpp>
#include <boost/filesystem.hpp>
#include <Exceptions/FileExceptions.hpp>
#include <Exceptions/RobotExceptions.hpp>
#include <ThirdParty/UTM.hpp>
#include <iostream>
#include <string> 

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
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

void Platform::updateState(VariableManager* manager)
{
    updateState(manager->getRedisState("gps.raw").asAffine());
}

void Platform::setRedisJsonStates(VariableManager* manager, State& rawState)
{
    // states
    manager->getStream().setRedisJsonValue("gps.raw.state", rawState.toJson(gps.utm_zone));                   
    manager->getStream().setRedisJsonValue("gps.ref.state", gps.getState().toJson(gps.utm_zone));                   
    manager->getStream().setRedisJsonValue("robot.ref.state", robot.getState().toJson(gps.utm_zone));                   
    manager->getStream().setRedisJsonValue("robot.center.state", robot.getCenterState().toJson(gps.utm_zone));                   
    manager->getStream().setRedisJsonValue("robot.head.state", robot.getHeadState().toJson(gps.utm_zone));                   

    // hitch
    json hitchRefStates = json();
    for (Hitch& h: hitches) {
        string entityName = h.getEntityName();
        double hitchAngle = h.updateAngle(manager);
        
        hitchRefStates[entityName] = h.toStateFullJson(hitchAngle, gps.utm_zone);
        hitchRefStates[entityName]["angle"] = hitchAngle;
        hitchRefStates[entityName]["height"] = h.updateHeight(manager);
        hitchRefStates[entityName]["busy"] = h.updateBusy(manager); 
        hitchRefStates[entityName]["activate"] = h.updateActivate(manager);       
    }
    manager->getStream().setRedisJsonValue("hitch.states", hitchRefStates);

    // contours
    json contours = json();
    Polygon polygonRobot;
    polygonRobot.update(robot.centerState.asAffine(), robot.width, robot.length);
    vector<vector<double>> robotLatLng;
    vector<vector<double>> robotXY;
    polygonRobot.contour(robotLatLng, robotXY, gps.utm_zone);
    contours["latlng"] = robotLatLng;
    contours["xy"] = robotXY;
    manager->getStream().setRedisJsonValue("robot.contour", contours);
}

void Platform::setRedisJsonStatus(VariableManager* manager)
{
    // status
    json errorJson;
    double distance_error = manager->getVariable("pc.path.distance_error")->getValue<double>();
    double navAbsError = abs(distance_error);
    stringstream ss;
    ss << std::fixed << std::setprecision(2);
    if (navAbsError <= 1.0) {
        ss << navAbsError * 100 << " cm";
    } else {
        ss << min(navAbsError, 99.0) << " m";
    }
    errorJson["value"] = ss.str();
    errorJson["positive"] = (distance_error > 0 ? navAbsError : 0);
    errorJson["negative"] = (distance_error < 0 ? navAbsError : 0);  

    json statusJson;
    statusJson["error"] = errorJson;
    statusJson["simulation_active"] = manager->getVariable("pc.simulation.active")->getValue<bool>();
    statusJson["fix"] = fixNumber[manager->getVariable("pc.gps.fix")->getValue<int>()];
    statusJson["notification"] = manager->getVariable("pc.execution.notification")->getValue<string>();
    statusJson["heartbeat"] = manager->getVariable("plc.control.navigation.heartbeat")->getValue<bool>();

    if (manager->existsVariable("plc.monitor.power_source.data.soc")) {
        statusJson["power_level"] = manager->getVariable("plc.monitor.power_source.data.soc")->getValue<double>();
    } else if (manager->existsVariable("plc.monitor.power_source.data.level")) {
        statusJson["power_level"] = manager->getVariable("plc.monitor.power_source.data.level")->getValue<double>();
    } else {
        statusJson["power_level"] = 0.0;
    }
    for (AutoMode state: auto_modes) {
        if (manager->getVariable("pc.simulation.active")->getValue<bool>()) {
           statusJson["current_state"] = manager->getVariable("pc.simulation.auto")->getValue<bool>() ? "auto" : "normal" ;
        } else {
            if (manager->getVariable("plc.monitor.state." + state.name)->getValue<bool>()) {
                statusJson["current_state"] = state.name;
                break;
            }
        }
    }
    manager->getStream().setRedisJsonValue("robot.status", statusJson);
}