#include <System/IlvoProcess.h>
#include <Utils/Logging/LoggerStream.h>
#include <Utils/Timing/Timing.h>
#include <iostream>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Utils::Timing;

using namespace nlohmann;
using namespace std;

namespace bp = boost::process;
namespace fs = boost::filesystem;

IlvoProcess::IlvoProcess(json ilvoProcess) : 
    IlvoJob(ilvoProcess), heartbeatHealthDetector(5s)
{
    if (ilvoProcess.contains("CheckHeartbeat")) checkHeartbeat = ilvoProcess["CheckHeartbeat"];
    else checkHeartbeat = false;
}

IlvoProcess::~IlvoProcess() {
    if (runs()) {
        stop();
    }
}

json IlvoProcess::toJson() {
    json jsonData = data.toJson();
    jsonData["CheckHeartbeat"] = checkHeartbeat;
    return jsonData;
}


void IlvoProcess::start() {
    fs::path programPath = bp::search_path(data.getName());
    if (programPath.string() != "") {
        process = bp::child(programPath);  // , bp::std_out > log_output); TODO std_out to log stream
        if (process.running()) {
            LoggerStream::getInstance() << INFO << "Process \"" << data.getName() << "\" started successfully.";
            data.setRunning(true);
        } else {
            LoggerStream::getInstance() << ERROR << "Failed to start process \"" << data.getName() << ". Exit code " << process.exit_code() << "\"";
            data.setRunning(false);
        }
        data.setPid(process.id());
        startTime = chrono::system_clock::now();
        data.setStartTimeISO(toISO8601Format(startTime)); // ISO 8601 format
        LoggerStream::getInstance() << INFO << "Started process \"" << data.getName() << "\"";
    } else {
        data.setErrorMessage("Program " + data.getName() + " not found");
        LoggerStream::getInstance() << WARN << "Failed to start process \"" << data.getName() << "\"";
        LoggerStream::getInstance() << ERROR << data.getErrorMessage();
    }
}

void IlvoProcess::stop() {
    if (runs()) {
        process.terminate();
        LoggerStream::getInstance() << INFO << "Stopped process \"" << data.getName() << "\"";
    } else {
        LoggerStream::getInstance() << INFO << "Process \"" << data.getName() << "\" was already stopped";
    }
    
    data.setRunning(false);
}

bool IlvoProcess::runs() {
    if (data.getPid() == -1) {
        data.setRunning(false); 
    } else {
        if (process.running()) {
            data.setRunning(true);
            data.setPid(process.id());
        } else {
            data.setRunning(false);
            data.setExitCode(process.exit_code());
            LoggerStream::getInstance() << ERROR << "Process \"" << data.getName() << "\" stopped with exit code " << process.exit_code();
        }
    }

    return data.getRunning();
}

bool IlvoProcess::heartbeatHealthy(bool heartbeatValue) {
    // Skip if heartbeat is not enabled
    if (!checkHeartbeat) {
        return true;
    }
    // Check if heartbeat is healthy
    if (heartbeatHealthDetector.expired(heartbeatValue)) {
        data.setRunning(false);
        LoggerStream::getInstance() << ERROR << "Process \"" << data.getName() << "\" stopped because no heartbeat was received";
        return false;
    } else {
        return true;
    }
}

void IlvoProcess::updateSoftware() {
    // Not necessary to implement
}

bool IlvoProcess::exists() {
    FILE* cmd = popen(("pidof " + data.getName()).c_str(), "r");
    if (!cmd) {
        LoggerStream::getInstance() << ERROR << "Error: Unable to execute pidof command.";
        return false;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), cmd)) {
        pclose(cmd);
        return true; // Process with the given name exists
    }

    pclose(cmd);
    return false; // No process with the given name found
}

void IlvoProcess::kill() {
    if (exists()) {
        int result = system(("killall " + data.getName()).c_str());
        if (result == 0) {
            LoggerStream::getInstance() << INFO << "Process " << data.getName() << " killed successfully.";
        } else {
            LoggerStream::getInstance() << ERROR << "Error: Unable to execute pidof command.";
        }
    } else {
        LoggerStream::getInstance() << INFO << "No process with name " << data.getName() << " found.";
    }
}