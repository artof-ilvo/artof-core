#pragma once

#include <Utils/Docker/DockerClient.h>
#include <Utils/Docker/DockerRegistry.h>
#include <System/IlvoJob.h>

#include <string>
#include <chrono>

namespace Ilvo {
namespace Core {
    class IlvoAddon: public IlvoJob
    {
    private:
        std::string containerId;
        std::string imageName;

        nlohmann::json dockerConfig;
        Ilvo::Utils::Docker::DockerRegistry dockerRegistry;
        Ilvo::Utils::Docker::DockerClient& dockerClient;

        /**
         * @brief Create a new docker container in the system if it does not exist already
         * 
         * @param imageName: name of the docker image
         */
        void createContainer(const std::string& imageName);

        /**
         * @brief Docker config with the bind sources below ILVO_PATH as host paths
         * 
         * Binds are configured as paths of this file system. When the system manager
         * runs in a container, the Docker daemon needs the host path of ILVO_PATH:
         * ILVO_HOST_PATH when set, otherwise the source of the ILVO_PATH mount.
         */
        nlohmann::json hostDockerConfig();
    public:
        IlvoAddon(nlohmann::json ilvoAddon, Utils::Docker::DockerClient& dockerClient);
        ~IlvoAddon();

        nlohmann::json toJson();

        bool runs();
        void start();
        void stop();
        
        void pull();
        void updateSoftware();
    };
    
}
}