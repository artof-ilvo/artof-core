/**
 * @file DockerClient.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Docker client based on: https://github.com/cqbqdd11519/DockerClient
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <ThirdParty/json.hpp>

namespace Ilvo {
namespace Utils {
namespace Docker {

typedef enum{
    GET,
    POST,
    DELETE,
    PUT
} Method;

std::string param( const std::string& param_name, const std::string& param_value);
std::string param( const std::string& param_name, const char* param_value);
std::string param( const std::string& param_name, bool param_value);
std::string param( const std::string& param_name, int param_value);
std::string param( const std::string& param_name, nlohmann::json& param_value);

class DockerClient{
    public :
        DockerClient();
        explicit DockerClient(std::string api_version);
        explicit DockerClient(std::string host, std::string api_version);
        ~DockerClient();

        /*
        * System
        */
        nlohmann::json system_info();
        nlohmann::json docker_version();

        /*
        * Images
        */
        nlohmann::json list_images();
        nlohmann::json pull_image(nlohmann::json& parameters, const std::string& image_name);
        /*
        * Containers
        */
        nlohmann::json list_containers(bool all=true, int limit=-1, int size=-1, nlohmann::json filters=nlohmann::json());
        nlohmann::json inspect_container(const std::string& container_id);
        nlohmann::json top_container(const std::string& container_id);
        nlohmann::json logs_container(const std::string& container_id, bool follow=false, bool o_stdout=true, bool o_stderr=false, bool timestamps=false, const std::string& tail="all");
        nlohmann::json create_container(nlohmann::json& parameters, const std::string& name="");
        nlohmann::json start_container(const std::string& container_id);
        nlohmann::json get_container_changes(const std::string& container_id);
        nlohmann::json stop_container(const std::string& container_id);
        nlohmann::json kill_container(const std::string& container_id, int signal=-1);
        nlohmann::json pause_container(const std::string& container_id);
        nlohmann::json wait_container(const std::string& container_id);
        nlohmann::json delete_container(const std::string& container_id, bool v=false, bool force=false);
        nlohmann::json unpause_container(const std::string& container_id);
        nlohmann::json restart_container(const std::string& container_id, int delay=-1);
        nlohmann::json attach_to_container(const std::string& container_id, bool logs=false, bool stream=false, bool o_stdin=false, bool o_stdout=false, bool o_stderr=false);

        bool exists_container(const std::string& container_id);

        const std::string& get_last_command() const;
    private:
        std::string last_command;
        std::string host_uri;
        std::string api_version{"v1.44"};
        bool is_remote;
        CURL *curl{};
        CURLcode res{};

        nlohmann::json requestAndParse(Method method, const std::string& path, unsigned success_code = 200, nlohmann::json add_body=nlohmann::json(), nlohmann::json add_headers=nlohmann::json());
        nlohmann::json requestAndParseJson(Method method, const std::string& path, unsigned success_code = 200, nlohmann::json add_body=nlohmann::json(), nlohmann::json add_headers=nlohmann::json());

        static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        }
};

}   // namespace Docker
}   // namespace Utils
}   // namespace Ilvo