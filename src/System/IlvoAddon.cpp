#include <System/IlvoAddon.h>
#include <Utils/Logging/LoggerStream.h>
#include <Utils/Timing/Timing.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Docker;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Utils::Timing;

using namespace std;
using namespace nlohmann;

IlvoAddon::IlvoAddon(nlohmann::json ilvoAddon, DockerClient& dockerClient) :
    IlvoJob(ilvoAddon),
    dockerClient(dockerClient),
    dockerConfig(ilvoAddon["DockerConfig"]),
    imageName(ilvoAddon["DockerConfig"]["Image"].get<string>()),
    containerId("")
{
    dockerRegistry.parse(ilvoAddon["DockerRegistry"]);
    if (ilvoAddon.contains("ContainerId")) {
        string id = ilvoAddon["ContainerId"].get<string>();
        if (dockerClient.exists_container(id)) {
            containerId = id;
        }
    }
}

IlvoAddon::~IlvoAddon() {
    if (runs()) {
        stop();
    }
}

json IlvoAddon::toJson() {
    json jReturn = data.toJson();
    jReturn["ContainerId"] = containerId;
    jReturn["DockerRegistry"] = dockerRegistry.toJson();
    jReturn["DockerConfig"] = dockerConfig;
    return jReturn;
}

void IlvoAddon::pull() {
    json docPull = dockerClient.pull_image(dockerRegistry.asHeader(), imageName);  
    if (docPull["code"] == 200) {
        LoggerStream::getInstance() << INFO <<"Pulled image: " << imageName << " successfully";
    } else {
        LoggerStream::getInstance() << WARN <<"Failed to pull image: " << imageName;
    }
}

void IlvoAddon::updateSoftware() {
    LoggerStream::getInstance() << DEBUG <<"Update image: " << imageName;
    // Stop container
    stop();
    // Pull container
    pull();
    // Remove container
    json docDel = dockerClient.delete_container(containerId); 
    if (docDel["code"] == 204) {
        LoggerStream::getInstance() << INFO <<"Deleted container: " << containerId;
        containerId = "";
    } else {
        LoggerStream::getInstance() << WARN <<"Failed to delete container: " << containerId << " with code " << docDel["code"];
        return;
    }
    // Start container
    start();
}

void IlvoAddon::createContainer(const std::string& imageName) {
    json docCreate = dockerClient.create_container(dockerConfig, data.getName()); 
    LoggerStream::getInstance() << DEBUG << "Create container code: " << docCreate["code"] ;
    if (docCreate["code"] == 201) {
        containerId = docCreate["data"]["Id"].get<string>();
        LoggerStream::getInstance() << INFO << "Created container \"" << data.getName() << "\" (" << containerId << ")";
    } else if (docCreate["code"] == 404 || docCreate["code"] == 409) {
        pull();
        createContainer(imageName);
    } else {
        data.setErrorMessage("Container creation of image " + imageName + " failed. Does the image exist?");
        LoggerStream::getInstance() << WARN << data.getErrorMessage();
        LoggerStream::getInstance() << WARN << "Command used: " << dockerClient.get_last_command();
        LoggerStream::getInstance() << WARN << docCreate.dump();
        LoggerStream::getInstance() << WARN <<"Is the config correct?";
        LoggerStream::getInstance() << WARN << dockerConfig.dump();
    }
}

void IlvoAddon::start() {
    // Search for containers of same image
    json docList = dockerClient.list_containers();
    if (docList["code"] == 200) {
        for (auto& container : docList["data"]) {
            bool equalImage = container["Image"] == imageName;
            auto names = container["Names"];
            bool equalName = std::find(names.begin(), names.end(), "/" + data.getName()) != names.end();
            if (equalImage && equalName) {
                containerId = container["Id"].get<string>();
                break;
            }
        }
    }

    // Create container when no containerId
    if (containerId.empty()) {
        string imageName = data.getName();
        createContainer(imageName);
    } else {
        LoggerStream::getInstance() << INFO << "Container \"" << data.getName() << "\" was found (" << containerId << ")";
    }

    // Start container
    json docStart = dockerClient.start_container(containerId); 
    LoggerStream::getInstance() << DEBUG << "Start container code: " << docStart["code"];
    if (docStart["code"] == 204) {
        LoggerStream::getInstance() << INFO << "Started container \"" << data.getName() << "\" (" << containerId << ")";
        startTime = chrono::system_clock::now();
        data.setStartTimeISO(toISO8601Format(startTime));  // ISO 8601 format
        data.setRunning(true);
    } else if (docStart["code"] == 304) {
        LoggerStream::getInstance() << INFO << "Container " << containerId << " was already running.";
    } else {
        LoggerStream::getInstance() << WARN << "Failed to start container: " << containerId;
    }
}

void IlvoAddon::stop() {
    json docStop = dockerClient.stop_container(containerId);
    if (docStop["code"] == 204) {
        LoggerStream::getInstance() << INFO << "Stopped container \"" << data.getName() << "\"";
    } else {
        LoggerStream::getInstance() << INFO << "Container \"" << data.getName() << "\" was already stopped";
    }
    data.setRunning(false);
}

bool IlvoAddon::runs() {
    if (containerId == "") {
        data.setRunning(false);
    } else {
        json docInspect = dockerClient.inspect_container(containerId);
        if (docInspect["success"] && docInspect["code"] == 200) {
            data.setExitCode(docInspect["data"]["State"]["ExitCode"]);
            data.setPid(docInspect["data"]["State"]["Pid"]);
            data.setRunning(docInspect["data"]["State"]["Running"]);  
        } else {
            data.setRunning(false);
        }
    }
    
    return data.getRunning();
}
