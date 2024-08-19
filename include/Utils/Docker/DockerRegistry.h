/**
 * @file DockerRegistry.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief 
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <ThirdParty/json.hpp>
#include <string>

namespace Ilvo {
namespace Utils {
namespace Docker {
    
    /** @brief Docker registry data container */
    class DockerRegistry
    {
    private:
        nlohmann::json registry;
        nlohmann::json authConfigHeader;
    public:
        DockerRegistry() = default;
        DockerRegistry(nlohmann::json registry);
        ~DockerRegistry() = default;

        void parse(nlohmann::json registry);
        nlohmann::json& asHeader();
        nlohmann::json toJson();
    };

}   // namespace Docker
}   // namespace Utils
}   // namespace Ilvo