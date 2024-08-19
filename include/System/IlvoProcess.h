/**
 * @file IlvoProcess.h
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
#include <System/IlvoJob.h>
#include <Utils/Timing/Logic.h>
#include <boost/process.hpp>

namespace Ilvo {
namespace Core {
    class IlvoProcess: public IlvoJob
    {
    private:
        bool checkHeartbeat;
        boost::process::ipstream pipe_stream;
        boost::process::child process;

        Utils::Timing::TimeEdgeDetector heartbeatHealthDetector;
    public:
        IlvoProcess(nlohmann::json ilvoProcess);
        ~IlvoProcess();

        nlohmann::json toJson();

        void kill();
        bool exists();

        std::string getName();
        int getExitCode();

        bool runs();
        bool heartbeatHealthy(bool heartbeatValue);
        void start();
        void stop();
        void updateSoftware();
    };
    
}
}