/**
 * @file IlvoJobData.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief 
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <chrono>

#include <ThirdParty/json.hpp>

namespace Ilvo {
namespace Core {

    /**
     * @brief Data related to a add-on (docker) or process (process)
     * 
     */
    class IlvoJobData
    {
        private:
            std::string name;
            int pid;
            int exitCode;
            bool running;
            bool startCommand;
            bool stopCommand;
            bool updateCommand;
            bool autoStart;
            std::string errorMessage;
            std::string startTimeISO;
        public:
            /**
             * @brief Construct a new Ilvo Job Data object based on the data extracted from the redis database.
             * 
             * @param ilvoJobData: data extracted from the redis database
             */
            IlvoJobData(nlohmann::json ilvoJobData);
            ~IlvoJobData() = default;

            nlohmann::json toJson();

            std::string getName();
            std::string getErrorMessage();
            bool getAutoStart();
            bool getStartCommand();
            bool getStopCommand();
            bool getUpdateCommand();
            bool getRunning();
            int getPid();
            int getExitCode();

            void setAutoStart(bool autoStart);
            void setStartCommand(bool startCommand);
            void setStopCommand(bool stopCommand);
            void setUpdateCommand(bool updateCommand);
            void setRunning(bool running);
            void setStartTimeISO(std::string startTimeISO);
            void setPid(int pid);
            void setErrorMessage(std::string errorMessage);
            void setExitCode(int exitCode);
    };
    
}
}