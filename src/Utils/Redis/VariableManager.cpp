#include <Utils/Redis/VariableManager.h>
#include <Utils/Redis/RedisStream.h>
#include <Utils/Settings/Task.h>
#include <Utils/Settings/Gps.h>
#include <Utils/String/String.h>
#include <Utils/Logging/LoggerStream.h>
#include <Exceptions/FileExceptions.hpp>
#include <Exceptions/RedisExceptions.hpp>
#include <boost/filesystem.hpp>
#include <ThirdParty/bprinter/table_printer.h>
#include <malloc.h>

using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Timing;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::String;
using namespace Ilvo::Exception;

using namespace std;
using namespace nlohmann;
using namespace Eigen;
using namespace boost::filesystem;
using namespace boost::algorithm;
using bprinter::TablePrinter;

// signal flag to quit the thread
atomic<bool> quit(false);    


void Ilvo::Utils::Redis::signalInterrupt(int) {
    quit.store(true);
}

// VariableManager
VariableManager::VariableManager(string processName) : 
    VariableManager(processName, 20ms)
{}


VariableManager::VariableManager(string processName, chrono::milliseconds processPeriod):
    processName(processName),
    clk(Clk{processPeriod}),
    platform(Platform::getInstance()),
    heartbeatPulse(500ms)
{
    LoggerStream::getInstance() << INFO << "### \t Welcome to the stdout of process \'" << processName << "\'! \t ###";

    // check config file
    string pathConfig = string(getenv("ILVO_PATH")) + "/config.json";
    if ( !exists(pathConfig) ) {
        throw PathNotFoundException(pathConfig);
    }
    string pathTypes = string(getenv("ILVO_PATH")) + "/types.json";
    if ( !exists(pathTypes) ) {
        throw PathNotFoundException(pathTypes);
    }
    try {
        jTypes = ordered_json::parse(std::ifstream(pathTypes));
    } catch(json::exception& e) {
        LoggerStream::getInstance() << ERROR << "Type file parse error, file path: \"" << pathTypes << "\", " << e.what();
        throw runtime_error("Type file parse error, " + std::string(e.what()));
    }
    try {
        jConfig = ordered_json::parse(std::ifstream(pathConfig));
    } catch(json::exception& e) {
        LoggerStream::getInstance() << ERROR << "Configuration file parse error, file path: \"" << pathConfig << "\", " << e.what();
        throw runtime_error("Configuration file parse error, " + std::string(e.what()));
    }
    // Setup redis stream
    rs = RedisStream(jConfig["protocols"]["redis"]);
    // Load variables
    this->load();
}

Platform& VariableManager::getPlatform()
{
    return platform;
}

void VariableManager::load()
{
    loadVariables("plc.monitor", jConfig["variables"]["plc"]["monitor"], PlcType::MONITOR);
    loadVariables("plc.control", jConfig["variables"]["plc"]["control"], PlcType::CONTROL);
    loadVariables("pc", jConfig["variables"]["pc"], PlcType::NONE);
    // Add the heartbeat variable to the map
    addVariable(getHeartbeatVariableName(processName), "pc", "execution", "bool", PlcType::NONE);
    // Propagate all default values of the variables that are nil in the redis database
    writeRedisVariables();
}

// Iterative completion of the variable map
void VariableManager::loadVariables(string name, const ordered_json& variable, PlcType plcType, string group, string entity)
{
    for (auto it = variable.begin(); it != variable.end(); ++it) {
        string key = it.key();
        if (it.value().is_string()) {
            string type = it.value();

            if (jTypes.contains(type)) {
                loadVariables(name + "." + key, jTypes[type], plcType, group.empty() ? name : group, entity.empty() ? key : entity);
            } else {
                // Effective addition of the variables
                if (type.find("array") != string::npos) {
                    // Process array variables
                    string arrayType = type.substr(type.find("of ") + 3);
                    string arraySizeStr = type.substr(type.find("[") + 1, type.find("]") - type.find("[") - 1);
                    int arraySize = stoi(arraySizeStr);

                    for (int i = 0; i < arraySize; i++) {
                        string variableName = name + "." + key + "." + to_string(i);
                        addVariable(variableName, group, entity, arrayType, plcType);
                    }
                } else {
                    // Process other variables
                    string variableName = name + "." + key;
                    addVariable(variableName, group, entity, type, plcType);
                }
            }
        } else {
            loadVariables(name + "." + key, it.value(), plcType);
        }
    }
}

void VariableManager::addVariable(string name, string group, string entity, string type, PlcType plcType)
{
    // add the variable to the map
    VariablePtr var = make_shared<Variable>(name, group, entity, type, plcType);
    variableMap.insert(pair<string, VariablePtr>(var->getName(), var));
    variableMapKeyOrder.push_back(var->getName());

    // initialize the variable that are nil to default value
    if (rs.isRedisValueNil(var->getName())) {
        var->setDefaultValue();
        var->setUpdated(true);
    }
}

void VariableManager::readRedisVariables()
{
    auto arr = rs.getRedisValues(variableMapKeyOrder);
    
    for (int i = 0; i < variableMapKeyOrder.size(); i++) {
        string key = variableMapKeyOrder[i];
        string valueStr{rediscpp::value(arr[i]).as_string()};

        bool valueIsNil = valueStr.empty();
        if (valueIsNil) {
            LoggerStream::getInstance() << INFO << "Variable \'" << key << "\' is (nil).";
        }

        VariablePtr var = variableMap.at(key);
        var->setValueString(valueStr);
    }
}

void VariableManager::writeRedisVariables()
{
    vector<string> values;

    for (int i = 0; i < variableMapKeyOrder.size(); i++) {
        VariablePtr var = getVariable(variableMapKeyOrder[i]);

        if (var->isUpdated()) {
            values.push_back(var->getName());
            values.push_back(var->getValueAsString());
            var->setUpdated(false);
        }
    }

    // Write heartbeat pulse
    values.push_back(getHeartbeatVariableName(processName));
    values.push_back((heartbeatPulse.getValue() ? "true" : "false"));

    rs.setRedisValues(values);
}

bool VariableManager::existsVariable(std::string key)  {
    return variableMap.count(key) > 0;
}

VariablePtr VariableManager::getVariable(string key) {
    string variableName = "";
    if (existsVariable(key)) {
        variableName = key;
    } else {
        for (const auto& keyName : variableMapKeyOrder) {
            if (keyName.find(key) != string::npos) {
                variableName = keyName;
                break;
            }
        }
    }

    if (variableName.empty()) {
        throw RedisNoSuchVariableException(key);
    }
    return variableMap[key];
}

void VariableManager::run()
{
    readRedisVariables();
    init();

    // signal handler
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = signalInterrupt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

    // main loop
    LoggerStream::getInstance() << DEBUG << "Starting main loop of VariableManager.";
    while (true) {
        clk.start();
        readRedisVariables();

        serverTick();

        getVariable(getHeartbeatVariableName(processName))->setValue<bool>(heartbeatPulse.generatePulse());

        writeRedisVariables();
        rs.publishRedisValue(processName + "-tick", clk.poll()); 
        clk.stop();

        if( quit.load() ) break;    // exit normally after SIGINT
    }
}

RedisStream& VariableManager::getStream()
{
    return rs;
}

void VariableManager::setRedisJsonStates(Platform& platform, State& rawState)
{
    // states
    rs.setRedisJsonValue("gps.raw.state", rawState.toJson(platform.gps.utm_zone));                   
    rs.setRedisJsonValue("gps.ref.state", platform.gps.getState().toJson(platform.gps.utm_zone));                   
    rs.setRedisJsonValue("robot.ref.state", platform.robot.getState().toJson(platform.gps.utm_zone));                   
    rs.setRedisJsonValue("robot.center.state", platform.robot.getCenterState().toJson(platform.gps.utm_zone));                   
    rs.setRedisJsonValue("robot.head.state", platform.robot.getHeadState().toJson(platform.gps.utm_zone));                   

    // hitch
    json hitchRefStates = json();
    for (Hitch& h: platform.hitches) {
        string entityName = h.getEntityName();
        string hitchAngleName = "plc.monitor." + entityName + ".angle";
        string hitchHeightName = "plc.monitor." + entityName + ".height";
        string hitchBusyName = "plc.monitor." + entityName + ".busy";
        string hitchActivateName = "plc.control." + entityName + ".activate";

        double hitchAngle =  existsVariable(hitchAngleName) ? getVariable(hitchAngleName)->getValue<double>() : 0.0;
        double hitchHeight =  existsVariable(hitchHeightName) ? getVariable(hitchHeightName)->getValue<double>() : 0.0;
        bool hitchBusy =  existsVariable(hitchBusyName) ? getVariable(hitchBusyName)->getValue<bool>() : false;
        bool hitchActivate =  existsVariable(hitchActivateName) ? getVariable(hitchActivateName)->getValue<bool>() : false;

        hitchRefStates[entityName] = h.toStateFullJson(hitchAngle, platform.gps.utm_zone);
        hitchRefStates[entityName]["angle"] = hitchAngle;
        hitchRefStates[entityName]["height"] = hitchHeight;
        hitchRefStates[entityName]["busy"] = hitchBusy; 
        hitchRefStates[entityName]["activate"] = hitchActivate;       
    }
    rs.setRedisJsonValue("hitch.states", hitchRefStates);

    // contours
    json contours = json();
    Polygon polygonRobot;
    polygonRobot.update(platform.robot.centerState.asAffine(), platform.robot.width, platform.robot.length);
    vector<vector<double>> robotLatLng;
    vector<vector<double>> robotXY;
    polygonRobot.contour(robotLatLng, robotXY, platform.gps.utm_zone);
    contours["latlng"] = robotLatLng;
    contours["xy"] = robotXY;
    rs.setRedisJsonValue("robot.contour", contours);
}

void VariableManager::setRedisJsonStatus(Platform& platform)
{
    // status
    json errorJson;
    double distance_error = getVariable("pc.path.distance_error")->getValue<double>();
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
    statusJson["simulation_active"] = getVariable("pc.simulation.active")->getValue<bool>();
    statusJson["fix"] = fixNumber[getVariable("pc.gps.fix")->getValue<int>()];
    statusJson["notification"] = getVariable("pc.execution.notification")->getValue<string>();
    statusJson["heartbeat"] = getVariable("plc.control.navigation.heartbeat")->getValue<bool>();

    if (existsVariable("plc.monitor.power_source.data.soc")) {
        statusJson["power_level"] = getVariable("plc.monitor.power_source.data.soc")->getValue<double>();
    } else if (existsVariable("plc.monitor.power_source.data.level")) {
        statusJson["power_level"] = getVariable("plc.monitor.power_source.data.level")->getValue<double>();
    } else {
        statusJson["power_level"] = 0.0;
    }
    for (AutoMode state: platform.auto_modes) {
        if (getVariable("plc.monitor.state." + state.name)->getValue<bool>()) {
            statusJson["current_state"] = state.name;
            break;
        }
    }
    rs.setRedisJsonValue("robot.status", statusJson);
}


State VariableManager::getRedisState(string name)
{
    string stateName = name + ".state";
    json jState = rs.getRedisJsonValue(stateName);
    if (jState.empty()) {
        State emptyState;
        // initialize redis json states
        rs.setRedisJsonValue(stateName, emptyState.toJson());
        LoggerStream::getInstance() << INFO << "State \'" << stateName << "\' is (nil). State has been initialized zero.";
        // return empty state
        return emptyState;
    } else {
        return State(jState);
    }
}

void VariableManager::updatePlatformState()
{
    platform.updateState(getRedisState("gps.raw").asAffine());
}
