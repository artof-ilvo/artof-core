#include <Exceptions/PlcExceptions.hpp>
#include <Utils/Redis/Plc.h>
#include <Utils/Timing/Timing.h>
#include <Utils/Logging/LoggerStream.h>
#include <iostream>

using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Timing;
using namespace Ilvo::Utils::Logging;

using namespace nlohmann;
using namespace std;

Plc::Plc(ordered_json& j) :
    rack(0),
    slot(1)
{
    // PLC settings
    if (j.contains("read_db")) readDb = j["read_db"];
    else throw PlcSettingsException("read_db");

    if (j.contains("write_db")) writeDb = j["write_db"];
    else throw PlcSettingsException("write_db");

    if (j.contains("ip")) ip = j["ip"].get<string>();
    else throw PlcSettingsException("ip");

    if (j.contains("rack")) rack = j["rack"];
    if (j.contains("slot")) slot = j["slot"];


    // PLC connection
    waitForNetworkConnection(ip, 10s);

    bool plcConnectionOk = false;
    bool firstTry = true;
    while (true) {
        ConnectTo(ip.c_str(), rack, slot);
        this_thread::sleep_for(1s);
        if (Connected()) {
            LoggerStream::getInstance() << INFO << "PLC connected succesfully on ip: " << ip;
            break;
        } else if (!firstTry) {
            LoggerStream::getInstance() << WARN << "PLC SNAP7 connection failed on ip " << ip;
            LoggerStream::getInstance() << WARN << "-- Check that the Read Size, Rack, Slot and Ip in the pLC correspond to the config.yaml";
            LoggerStream::getInstance() << WARN << "-- Enable PUT_GET settings in PLC and turn off Optimized block access in PLC.";
            this_thread::sleep_for(5s);
        }
        firstTry = false;
    }
}

Plc::~Plc() 
{
    Disconnect();
}