#include <System/IlvoJobData.h>
#include <Utils/Logging/LoggerStream.h>
#include <iostream>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Logging;

using namespace std;
using namespace nlohmann;

IlvoJobData::IlvoJobData(json ilvoJob) 
{
    name = ilvoJob["Name"].get<string>();
    if (ilvoJob.contains("Pid")) pid = ilvoJob["Pid"];
    else pid = -1;
    if (ilvoJob.contains("ExitCode")) exitCode = ilvoJob["ExitCode"];
    else exitCode = -1;
    if (ilvoJob.contains("AutoStart")) autoStart = ilvoJob["AutoStart"];
    else autoStart = true;
    if (ilvoJob.contains("Running")) running = ilvoJob["Running"];
    else running = false;
    if (ilvoJob.contains("ErrorMessage")) errorMessage = ilvoJob["ErrorMessage"].get<string>();
    else errorMessage = "";
    if (ilvoJob.contains("UpdateCommand")) updateCommand = ilvoJob["UpdateCommand"];
    else updateCommand = false;
    if (ilvoJob.contains("StartCommand")) startCommand = ilvoJob["StartCommand"];
    else startCommand = false;
    if (ilvoJob.contains("StopCommand")) stopCommand = ilvoJob["StopCommand"];
    else stopCommand = false;
}

json IlvoJobData::toJson() {
    json jReturn;
    jReturn["Name"] = name;
    jReturn["Pid"] = pid;
    jReturn["ExitCode"] = exitCode;
    jReturn["AutoStart"] = autoStart;
    jReturn["Running"] = running;
    jReturn["StartCommand"] = startCommand;
    jReturn["StopCommand"] = stopCommand;
    jReturn["StartTimeISO"] = startTimeISO;
    jReturn["ErrorMessage"] = errorMessage;
    jReturn["UpdateCommand"] = updateCommand;
    return jReturn;
}

std::string IlvoJobData::getName() {
    return name;
}

std::string IlvoJobData::getErrorMessage() {
    return errorMessage;
}

bool IlvoJobData::getAutoStart() {
    return autoStart;
}

bool IlvoJobData::getStartCommand() {
    return startCommand;
}

bool IlvoJobData::getStopCommand() {
    return stopCommand;
}

bool IlvoJobData::getUpdateCommand() {
    return updateCommand;
}

bool IlvoJobData::getRunning() {
    return running;
}

int IlvoJobData::getPid() {
    return pid;
}

int IlvoJobData::getExitCode() {
    return exitCode;
}

void IlvoJobData::setAutoStart(bool autoStart) {
    this->autoStart = autoStart;
}

void IlvoJobData::setRunning(bool running) {
    this->running = running;
}

void IlvoJobData::setStartCommand(bool startCommand) {
    this->startCommand = startCommand;
}

void IlvoJobData::setStopCommand(bool stopCommand) {
    this->stopCommand = stopCommand;
}

void IlvoJobData::setUpdateCommand(bool updateCommand) {
    this->updateCommand = updateCommand;
}

void IlvoJobData::setStartTimeISO(std::string startTimeISO) {
    this->startTimeISO = startTimeISO;
}

void IlvoJobData::setPid(int pid) {
    this->pid = pid;
}

void IlvoJobData::setErrorMessage(std::string errorMessage) {
    this->errorMessage = errorMessage;
}

void IlvoJobData::setExitCode(int exitCode) {
    this->exitCode = exitCode;
}