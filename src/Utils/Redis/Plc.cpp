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

// PLC base class

Plc::Plc(std::string ip) : ip(ip)
{
}

Plc::Plc(ordered_json& j) : Plc(
    getRequired<std::string>(j, "ip"))
{
}

const string& Plc::getIp()  
{
    return ip;
}

bool Plc::connected()
{
    return true;
}

// S7 PLC class
S7Plc::S7Plc(std::string ip, int readDb, int writeDb, uint8_t rack, uint8_t slot) : Plc(ip), readDb(readDb), writeDb(writeDb), rack(rack), slot(slot)
{
    // PLC connection
    waitForNetworkConnection(ip, 10s);

    bool plcConnectionOk = false;
    bool firstTry = true;
    while (true) {
        plcClient.ConnectTo(ip.c_str(), rack, slot);
        this_thread::sleep_for(1s);
        if (plcClient.Connected()) {
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

S7Plc::S7Plc(ordered_json& j) : S7Plc(
    getRequired<std::string>(j, "ip"),
    getRequired<int>(j, "read_db"),
    getRequired<int>(j, "write_db"),
    getRequired<uint8_t>(j, "rack"),
    getRequired<uint8_t>(j, "slot"))
{
}

S7Plc::~S7Plc() 
{
    plcClient.Disconnect();
}

void S7Plc::read(int size, unsigned char* data)
{
    longword err = plcClient.DBRead(readDb, 0, size, data); 
    if (err != 0) {
        throw PlcReadException(err);
    }
}

void S7Plc::write(int size, unsigned char* data)
{
    longword err = plcClient.DBWrite(writeDb, 0, size, data); 
    if (err != 0) {
        throw PlcReadException(err);
    }
}

bool S7Plc::connected()
{
    return plcClient.Connected();
}

// UDP PLC class
UdpPlc::UdpPlc(std::string ip, int readPort, int writePort) : Plc(ip), 
    socketRead(io), socketWrite(io),
    readPort(readPort), writePort(writePort), 
    readerEndpoint(boost::asio::ip::address::from_string("0.0.0.0"), readPort),
    writerEndpoint(boost::asio::ip::address::from_string(ip), writePort)
{
    // Open read socket
    socketRead.open(readerEndpoint.protocol());
    socketRead.bind(readerEndpoint);

    // Open write socket
    socketWrite.open(writerEndpoint.protocol());
}

UdpPlc::UdpPlc(ordered_json& j) :
    UdpPlc(
        getRequired<std::string>(j, "ip"),
        getRequired<int>(j, "read_port"),
        getRequired<int>(j, "write_port"))
{
}

UdpPlc::~UdpPlc()
{
    socketRead.close();
    socketWrite.close();
}

void UdpPlc::read(int size, unsigned char* data)
{
    size_t len = socketRead.receive_from(boost::asio::buffer(data, size), readerEndpoint);
    std::cout << "Received " << len << " bytes from PLC via UDP." << std::endl;
    // Print the variables for debugging
    for (size_t i = 0; i < len; i++) {
        std::cout << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::endl;
    
    // size_t len = socketRead.receive_from(boost::asio::buffer(data, size), readerEndpoint);
    // std::cout << "Received " << len << " bytes from PLC via UDP." << std::endl;

    // if (len != size) {
    //     throw PlcReadException(len);
    // }
}

void UdpPlc::write(int size, unsigned char* data)
{
    // Check if endpoint is valid before sending
    std::cout << "Endpoint: " << writerEndpoint.address() << ":" 
            << writerEndpoint.port() << std::endl;

    size_t len = socketWrite.send_to(boost::asio::buffer(data, size), writerEndpoint);
    std::cout << "Sent " << len << " bytes to PLC via UDP." << std::endl;
}
