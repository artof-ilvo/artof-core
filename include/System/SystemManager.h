/**
 * @file SystemManager.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief 
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Redis/VariableManager.h>
#include <Utils/Docker/DockerClient.h>
#include <Utils/Timing/Logic.h>
#include <System/IlvoProcess.h>
#include <System/IlvoAddon.h>

#include <string>
#include <memory>
#include <map>
#include <chrono>


namespace Ilvo {
namespace Core {
    /**
     * @brief Maintains processes and add-ons in the system
     * 
     * @details The system manager maintains the processes and add-ons in the system based on configuration in the Redis database ('ilvoAddons' and 'ilvoProcesses').
     * It enables the user to start and stop processes and add-ons, as well as update the software or alter configuration.
     */
    class SystemManager: public Utils::Redis::VariableManager 
    {
    private:
        bool running;
        bool updateCommand;
        std::string startTimeISO;
        std::chrono::system_clock::time_point startTime;

        Utils::Timing::SinglePulseGenerator fieldPulseGenerator;

        /**
         * @brief Update the entire software package
         * 
         * @details TODO: This updates the new process binaries on the system. 
         */
        void updateSoftware();
        Utils::Docker::DockerClient dockerClient;

        std::map<std::string, std::unique_ptr<IlvoProcess>> processes = {};
        std::map<std::string, std::unique_ptr<IlvoAddon>> addons = {};

        void restartContainer(const std::string containerName);
        void updateContainer(const std::string containerName);
    public:
        SystemManager(const std::string ns);
        ~SystemManager();

        /**
         * @brief Process the system configuration extracted from the redis database
         * 
         * @param systemConfigJson: system configuration extracted from the redis database 
         * @return true: the system configuration was changed
         * @return false: the system configuration was not changed 
         */
        bool processJson(nlohmann::json systemConfigJson);
        /**
         * @brief Format the system configuration in a json format
         * 
         * @details The returned json is ready to be sent to the redis database again.
         * 
         * @return nlohmann::json 
         */
        nlohmann::json formatJson();

        void init() override;
        void serverTick() override;
    };
}
}