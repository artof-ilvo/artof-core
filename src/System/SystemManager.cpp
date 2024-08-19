#include <iostream>
#include <string>
#include <chrono>
#include <System/SystemManager.h>
#include <Utils/Logging/LoggerStream.h>
#include <Utils/String/String.h>
#include <Exceptions/FileExceptions.hpp>
#include <boost/filesystem.hpp>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Docker;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::String;

using namespace std;
using namespace nlohmann;
using namespace boost::filesystem;

SystemManager::SystemManager(const string ns) : 
    VariableManager(ns, 400ms),  // Make sure this is smaller than the heartbeat period
    running(true),
    startTime(chrono::system_clock::now()),
    dockerClient(Utils::Docker::DockerClient())
{
    // format ISO string
    time_t timet = chrono::system_clock::to_time_t(startTime);
    startTimeISO = (stringstream() << std::put_time(gmtime(&timet), "%FT%T")).str();
}

SystemManager::~SystemManager() {
    // Stop all processes
    for (auto process = processes.begin(); process != processes.end(); process++) {
        process->second->stop();
        LoggerStream::getInstance() << INFO << "Stopped process: " << process->first;
    }
    for (auto addon = addons.begin(); addon != addons.end(); addon++) {
        addon->second->stop();
        LoggerStream::getInstance() << INFO << "Stopped addon: " << addon->first;
    }

    // set variables
    running = false;

    // Update redis with final json
    rs.setRedisJsonValue("system", formatJson());  
}


void SystemManager::updateSoftware() {
    // TODO
    updateCommand = false;
}

void SystemManager::serverTick() {
    // System
    if (processJson(rs.getRedisJsonValue("system"))) {
        rs.setRedisJsonValue("system", formatJson());   
    }
    // Field
    if (getVariable("pc.field.updated")->getValue<bool>()) {
        getVariable("pc.field.updated")->setValue<bool>(fieldPulseGenerator.generatePulse(500ms));
    }
}

bool SystemManager::processJson(json systemConfigJson) {
    bool updated = false;

    if (!systemConfigJson.empty()) {
        if (systemConfigJson.contains("UpdateCommand")) updateCommand = systemConfigJson["UpdateCommand"];
        if (updateCommand) {
            updateSoftware();
            updated = true;
        }

        for (auto ilvoProcessJson : systemConfigJson["ilvoProcesses"]) {
            string ilvoProcessName = ilvoProcessJson["Name"];
            if (!processes.count(ilvoProcessName)) {  // Necessary when new process is added
                processes.insert({ilvoProcessName, std::make_unique<IlvoProcess>(ilvoProcessJson)});
                updated = true;
            } else {
                // Check if process is running
                string heartbeatStr = rs.getRedisValue(getHeartbeatVariableName(ilvoProcessName));
                bool heartbeat = (heartbeatStr == "true");
                // cout << "Heartbeat: " << (heartbeat ? "true" : "false") << ", HeartbeatStr: " << heartbeatStr << endl;
                if (!processes[ilvoProcessName]->runs() || !processes[ilvoProcessName]->heartbeatHealthy(heartbeat)) {
                    // stop process
                    processes[ilvoProcessName]->stop();
                    // start process
                    processes[ilvoProcessName]->start();
                    // process is updated
                    updated = true;
                }
            }
            
            if (processes[ilvoProcessName]->update(ilvoProcessJson)) {
                updated = true;
            }
        }
        for (auto ilvoAddonJson : systemConfigJson["ilvoAddons"]) {
            if (!addons.count(ilvoAddonJson["Name"])) {  // Necessary when new process is added
                addons.insert({ilvoAddonJson["Name"], std::make_unique<IlvoAddon>(ilvoAddonJson, dockerClient)});
                updated = true;
            }
            if (addons[ilvoAddonJson["Name"]]->update(ilvoAddonJson)) {
                updated = true;
            }
        }
    }

    return updated;
}

json SystemManager::formatJson() {
    json systemJson;
    for (auto process = processes.begin(); process != processes.end(); process++) {
        systemJson["ilvoProcesses"].push_back(process->second->toJson());
    }
    for (auto addon = addons.begin(); addon != addons.end(); addon++) {
        systemJson["ilvoAddons"].push_back(addon->second->toJson());
    }
    systemJson["Running"] = running;
    systemJson["UpdateCommand"] = updateCommand;
    systemJson["StartTimeISO"] = startTimeISO;

    return systemJson;
}

void SystemManager::init() {
    LoggerStream::getInstance() << INFO << "Initializing system manager";
    json systemConfigJson = rs.getRedisJsonValue("system");

    // Initialize redis variables for the first time
    if (systemConfigJson.empty()) {
        LoggerStream::getInstance() << INFO << "Initializing redis variables for the first time";

        // Initialize robot and implement states empty
        LoggerStream::getInstance() << INFO << "Initialize robot states empty.";
        State emptyState;
        setRedisJsonStates(platform, emptyState);
        setRedisJsonStatus(platform);  

        // This is the first start up time initilize everything properly
        path p(string(getenv("ILVO_PATH")) + "/redis.init.json");
        if (!exists(p)) { throw runtime_error("File not found: " + p.string()); }

        // read configuration file
        std::ifstream ifs(p);
        json configJson;
        try {
            configJson = json::parse(ifs);
        } catch(json::exception& e) {
            LoggerStream::getInstance() << ERROR << "Configuration file parse error, file path: \"" << p << "\", " << e.what();
            throw runtime_error("Configuration file parse error, " + std::string(e.what()));
        }
        systemConfigJson = configJson["system"];
        ifs.close();

        // initialize the redis variables
        json variables = configJson["variables"];
        if (configJson.contains("variables")) {
            readRedisVariables();
            for (auto& el : variables.items()) {
                VariablePtr var = getVariable(el.key());
                auto data = variables[el.key()];
                if (var->getType() == "string") {
                    var->setValue(data.get<string>());
                } else if (var->getType() == "bool") {
                    var->setValue(data.get<bool>());
                } else {
                    var->setValue(data.get<double>());
                } 
                LoggerStream::getInstance() << INFO << "Initialized variable: " << el.key() << " with value: " << var->getValueAsString();
            }
            writeRedisVariables();
        }
    } else {
        LoggerStream::getInstance() << INFO << "Redis variables have already been initialized";
    }

    for (auto ilvoProcessJson : systemConfigJson["ilvoProcesses"]) {
        std::unique_ptr<IlvoProcess> ilvoProcess = std::make_unique<IlvoProcess>(ilvoProcessJson);
        ilvoProcess->kill();
        if (ilvoProcess->getData().getAutoStart()) {
            ilvoProcess->start();
        }
        processes.insert({ilvoProcessJson["Name"], std::move(ilvoProcess)});
        this_thread::sleep_for(100ms);  // Sleep a bit to make sure the process is started
    }
    for (auto ilvoAddonJson : systemConfigJson["ilvoAddons"]) {
        std::unique_ptr<IlvoAddon> ilvoAddon = std::make_unique<IlvoAddon>(ilvoAddonJson, dockerClient);
        if (ilvoAddon->getData().getAutoStart()) {
            ilvoAddon->start();
        }
        addons.insert({ilvoAddonJson["Name"], std::move(ilvoAddon)});
    }

    // Save system to redis
    rs.setRedisJsonValue("system", formatJson());   
}

int main() {
    // First check if ILVO_PATH environment variable is set
    if (getenv("ILVO_PATH") == NULL) { 
        throw EnvVariableNotFoundException("$ILVO_PATH");
    }

    string procName = "ilvo-system-manager";
    LoggerStream::createInstance(procName, true);
    LoggerStream::getInstance() << INFO << "System Manager Logger started";
    SystemManager SystemManager(procName);
    SystemManager.run();
    return 0;    
}