#include <System/IlvoJob.h>
#include <System/IlvoJobData.h>
#include <Utils/Logging/LoggerStream.h>
#include <iostream>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Logging;

using namespace std;
using namespace nlohmann;

IlvoJob::IlvoJob(json ilvoJob) : 
    data(IlvoJobData(ilvoJob))
{}

bool IlvoJob::update(IlvoJobData updateJob) {
    bool updated = false;
    // Check if job is running
    data.setRunning(runs());

    if (data.getRunning()) {
        // Check if job is still running
        if (updateJob.getStopCommand()) {
            stop();
            updated = true;
        }
        data.setStopCommand(false);  // Reset command
    } else {
        // Check if job should be started
        if (updateJob.getStartCommand()) {
            start();
            updated = true;
        }
        data.setStartCommand(false);  // Reset command
    }

    // Peform software update
    if (updateJob.getUpdateCommand()) {
        LoggerStream::getInstance() << DEBUG <<"Update command was set in job: " << data.getName();
        updateSoftware();
        updated = true;
    }
    data.setUpdateCommand(false);  // Reset command

    return updated;
}

IlvoJobData& IlvoJob::getData()
{
    return data;
}