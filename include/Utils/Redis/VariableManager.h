#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <atomic>
#include <map>
#include <Utils/Redis/Variable.h>
#include <Utils/Redis/RedisStream.h>
#include <Utils/String/String.h>
#include <Exceptions/RedisExceptions.hpp>
#include <Utils/Settings/Platform.h>
#include <Utils/Timing/Logic.h>

namespace Ilvo {
namespace Utils {
namespace Redis {

    typedef std::map<std::string, VariablePtr> VariableMap;


    /** @brief Signal handler function to handle proper shutdown of program */
    void signalInterrupt(int);

    class VariableManager
    {
    protected:
        /** @brief Name of the process */
        std::string processName;
        /** @brief Period of the process */
        Utils::Timing::Clk clk;
        /** @brief Platform settings */
        Utils::Settings::Platform& platform;

        /** 
         * @brief Pulse generator for heartbeat
         * 
         * @details A heartbeat pulse is generated. This pulse signal is used by the PLC to indicate that the robot is online.
         * If the heartbeat pulse is not detected by the PLC, their is assumed to be something wrong and the PLC will 
         * immediately stop autonomous navigation.
         */
        Utils::Timing::PulseGenerator heartbeatPulse;

        RedisStream rs;
        /** @brief Map of redis variables */
        VariableMap variableMap;
        /** @brief Map of redis variable keys for ordering */
        std::vector<std::string> variableMapKeyOrder;

        /** @brief Composed variable types defined in configuration json file */
        nlohmann::ordered_json jTypes;
        /** @brief Redis configuration defined in configuration json file */
        nlohmann::ordered_json jConfig;
    private:
        // load variables
        /** @brief Load redis variables */
        void load();
        void loadVariables(std::string name, const nlohmann::ordered_json& variable, PlcType plcType=PlcType::NONE, std::string group="", std::string entity="");
        void addVariable(std::string name, std::string group, std::string entity, std::string type, PlcType plcType);
    public:
        VariableManager(std::string processName, std::chrono::milliseconds processPeriod);
        VariableManager(std::string processName);
        virtual ~VariableManager() = default;
        
        // platform
        Utils::Settings::Platform& getPlatform();

        // Variable getters and setters by type
        /** @brief Read all redis variables */
        void readRedisVariables();
        /** @brief Write redis variables (only those that have been updated) */
        void writeRedisVariables();

        /** @brief Get a redis variable by key */
        VariablePtr getVariable(std::string key);

        /** @brief Check if a redis variable key exists */
        bool existsVariable(std::string key);
        
        /** @brief Set redis json states */
        void setRedisJsonStates(Settings::Platform& platform, Settings::State& rawState); 
        /** @brief Set redis json states */
        void setRedisJsonStatus(Settings::Platform& platform); 
        /** @brief Get redis json states */
        Settings::State getRedisState(std::string name);

        /** @brief Update platform state (center, hitches, etc) */
        void updatePlatformState();

        RedisStream& getStream();

        // pure virtual for operation
        virtual void serverTick() = 0;
        virtual void init() {};
        void run();
    };

    typedef std::shared_ptr<VariableManager> VariableManagerPtr;
} // Redis
} // Utils
} // Ilvo
