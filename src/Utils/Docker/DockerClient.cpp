#include <Utils/Docker/DockerClient.h>
#include <Utils/Logging/LoggerStream.h>
#include <utility>
#include <sstream>

using namespace Ilvo::Utils::Docker;
using namespace Ilvo::Utils::Logging;
using namespace nlohmann;

/*
*  
* START DockerClient Implementation
* 
*/

DockerClient::DockerClient() {
    if (getenv("ILVO_DOCKER_API_VERSION")) {
        api_version = std::string(getenv("ILVO_DOCKER_API_VERSION"));
    }
    host_uri = "http:/" + api_version;
    curl_global_init(CURL_GLOBAL_ALL);
    is_remote = false;
}

DockerClient::DockerClient(std::string api_version) {
    host_uri = "http:/" + api_version;
    curl_global_init(CURL_GLOBAL_ALL);
    is_remote = false;
}

DockerClient::DockerClient(std::string host, std::string api_version) : host_uri(){
    host_uri = std::move(host) + "/" + api_version;
    curl_global_init(CURL_GLOBAL_ALL);
    is_remote = true;
}

DockerClient::~DockerClient(){
    curl_global_cleanup();
}


/*
*  
* Public Methods
* 
*/

/*
* System
*/
json DockerClient::system_info(){
    std::string path = "/info";
    return requestAndParseJson(GET,path);
}
json DockerClient::docker_version(){
    std::string path = "/version";
    return requestAndParseJson(GET,path);
}

/*
* Images
*/
json DockerClient::list_images(){
    std::string path = "/images/json";
    return requestAndParseJson(GET,path);
}

json DockerClient::pull_image(json& headers, const std::string& image_name){
    std::string path = "/images/create?";
    headers["Content-Type"] = "application/tar";
    path += Ilvo::Utils::Docker::param("fromImage", image_name);
    path += Ilvo::Utils::Docker::param("tag", "latest");
    return requestAndParseJson(POST,path,200,json(),headers);
}

/*
* Containers
*/
json DockerClient::list_containers(bool all, int limit, int size, json filters){
    std::string path = "/containers/json?";
    path += Ilvo::Utils::Docker::param("all", all);
    if (limit > 0) path += Ilvo::Utils::Docker::param("limit", limit);
    if (size > 0) path += Ilvo::Utils::Docker::param("size", size);
    if (!filters.empty()) path += Ilvo::Utils::Docker::param("filters", filters);
    return requestAndParseJson(GET,path);
}
json DockerClient::inspect_container(const std::string& container_id){
    std::string path = "/containers/" + container_id + "/json";
    return requestAndParseJson(GET,path);
}
json DockerClient::top_container(const std::string& container_id){
    std::string path = "/containers/" + container_id + "/top";
    return requestAndParseJson(GET,path);
}
json DockerClient::logs_container(const std::string& container_id, bool follow, bool o_stdout, bool o_stderr, bool timestamps, const std::string& tail){
    std::string path = "/containers/" + container_id + "/logs?";
    path += Ilvo::Utils::Docker::param("follow", follow);
    path += Ilvo::Utils::Docker::param("stdout", o_stdout);
    path += Ilvo::Utils::Docker::param("stderr", o_stderr);
    path += Ilvo::Utils::Docker::param("timestamps", timestamps);
    path += Ilvo::Utils::Docker::param("tail", tail);
    return requestAndParse(GET,path,101);
}
json DockerClient::create_container(json& body, const std::string& name){
    std::string path = "/containers/create";
    path += not name.empty() ? "?name=" + name : "";
    return requestAndParseJson(POST,path,201,body);
}
json DockerClient::start_container(const std::string& container_id){
    std::string path = "/containers/" + container_id + "/start";
    return requestAndParse(POST,path,204);
}
json DockerClient::get_container_changes(const std::string& container_id){
    std::string path = "/containers/" + container_id + "/changes";
    return requestAndParseJson(GET,path);
}
json DockerClient::stop_container(const std::string& container_id){
    std::string path = "/containers/" + container_id + "/stop";
    return requestAndParse(POST,path,204);
}
json DockerClient::kill_container(const std::string& container_id, int signal){
    std::string path = "/containers/" + container_id + "/kill?";
    path += Ilvo::Utils::Docker::param("signal", signal);
    return requestAndParse(POST,path,204);
}
json DockerClient::pause_container(const std::string& container_id){
    std::string path = "/containers/" + container_id + "/pause";
    return requestAndParse(POST,path,204);
}
json DockerClient::wait_container(const std::string& container_id){
    std::string path = "/containers/" + container_id + "/wait";
    return requestAndParseJson(POST,path);
}
json DockerClient::delete_container(const std::string& container_id, bool v, bool force){
    std::string path = "/containers/" + container_id + "?";
    path += Ilvo::Utils::Docker::param("v", v);
    path += Ilvo::Utils::Docker::param("force", force);
    return requestAndParse(DELETE,path,204);
}
json DockerClient::unpause_container(const std::string& container_id){
    std::string path = "/containers/" + container_id + "/unpause?";
    return requestAndParse(POST,path,204);
}
json DockerClient::restart_container(const std::string& container_id, int delay){
    std::string path = "/containers/" + container_id + "/restart?";
    path += Ilvo::Utils::Docker::param("t", delay);
    return requestAndParse(POST,path,204);
}
json DockerClient::attach_to_container(const std::string& container_id, bool logs, bool stream, bool o_stdin, bool o_stdout, bool o_stderr){
    std::string path = "/containers/" + container_id + "/attach?";
    path += Ilvo::Utils::Docker::param("logs", logs);
    path += Ilvo::Utils::Docker::param("stream", stream);
    path += Ilvo::Utils::Docker::param("stdin", o_stdin);
    path += Ilvo::Utils::Docker::param("stdout", o_stdout);
    path += Ilvo::Utils::Docker::param("stderr", o_stderr);

    return requestAndParse(POST,path,101);
}
//void DockerClient::copy_from_container(const std::string& container_id, const std::string& file_path, const std::string& dest_tar_file){}

bool DockerClient::exists_container(const std::string& container_id){
    json docList = list_containers(); 
    for (auto& container : docList["data"]) {
        if(container["Id"].get<std::string>().find(container_id) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/*
*  
* Private Methods
* 
*/

json DockerClient::requestAndParse(Method method, const std::string& path, unsigned success_code, json add_body, json add_headers){
    std::string readBuffer;
    std::string method_str;

    struct curl_slist *headers = nullptr;
    std::string body_str = std::string(add_body.empty() ? "" : add_body.dump());

    switch(method){
        case GET:
            method_str = "GET";
            break;
        case POST:
            method_str = "POST";
            break;
        case DELETE:
            method_str = "DELETE";
            break;
        case PUT:
            method_str = "PUT";
            break;
        default:
            method_str = "GET";
    }

    curl = curl_easy_init();
    if(!curl){
        LoggerStream::getInstance() << WARN << "error while initiating curl";
        curl_global_cleanup();
        exit(1);
    }

    if(!add_headers.empty()) {
        for (auto& h : add_headers.items()) {
            headers = curl_slist_append(headers, (h.key() + ": " + h.value().get<std::string>()).c_str());
        }
    } else {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }

    if(!is_remote) {
        curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, "/var/run/docker.sock");
        last_command = "curl -s --unix-socket /var/run/docker.sock " + host_uri + path;
    } else {
        last_command = "curl " + host_uri + path;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, (host_uri + path).c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method_str.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    if(method == POST){
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body_str.length());
    }

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
        LoggerStream::getInstance() << WARN << "curl_easy_perform() failed: " << curl_easy_strerror(res);
    unsigned status = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);

    json doc;
    if(status == success_code || status == 200){
        doc["success"] = true;
        try {
            doc["data"] = json::parse(readBuffer.length() ? readBuffer : "{}");
        } catch (json::parse_error& e) {
            doc["data"] = readBuffer;
        }
        doc["code"] = status;
    }else{
        json resp;
        doc["success"] = false;
        doc["code"] = status;
        doc["data"] = resp;
    }
    curl_slist_free_all(headers);
    return doc;
}

json DockerClient::requestAndParseJson(Method method, const std::string& path, unsigned success_code, json add_body, json add_headers){
    if (add_headers.empty()) {
        add_headers["Accept"] = "application/json";
        add_headers["Content-Type"] = "application/json";
    }
    return requestAndParse(method,path,success_code,add_body,add_headers);
}

const std::string& DockerClient::get_last_command() const {
    return last_command;
}

/*
*  
* END DockerClient Implementation
* 
*/

std::string Ilvo::Utils::Docker::param( const std::string& param_name, const std::string& param_value){
    if(!param_value.empty()){
        return "&" + param_name + "=" + param_value;
    }
    else{
        return "";
    }
}

std::string Ilvo::Utils::Docker::param( const std::string& param_name, const char* param_value){
    if(param_value != nullptr){
        return "&" + param_name + "=" + param_value;
    }
    else{
        return "";
    }
}

std::string Ilvo::Utils::Docker::param( const std::string& param_name, bool param_value){
    std::string ret;
    ret = "&" + param_name + "=";
    if(param_value){
        return ret + "true";
    }
    else{
        return ret + "false";
    }
}

std::string Ilvo::Utils::Docker::param( const std::string& param_name, int param_value){
    if(param_value != -1){
        std::ostringstream convert;
        convert << param_value;
        return "&" + param_name + "=" + convert.str();
    }
    else{
        return "";
    }
}

std::string Ilvo::Utils::Docker::param( const std::string& param_name, json& param_value){
    if(!param_value.is_null()){
        return "&" + param_name + "=" + param_value[param_name].dump();
    }
    else{
        return "";
    }
}
