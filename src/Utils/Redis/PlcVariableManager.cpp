#include <Utils/Redis/PlcVariableManager.h>
#include <Utils/String/String.h>
#include <ThirdParty/snap7/snap7.h>
#include <ThirdParty/ieee754_types.hpp>
#include <cstddef>
#include <format> 
#include <string>
#include <Exceptions/PlcExceptions.hpp>
// https://github.com/dattanchu/bprinter/wiki
#include <ThirdParty/bprinter/table_printer.h>

using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::String;
using namespace Ilvo::Utils::Logging;

using namespace nlohmann;
using namespace std;
using namespace Ilvo::Exception;
using bprinter::TablePrinter;

// TODO make it an abstract class whereby different plc types can be used

PlcVariableManager::PlcVariableManager(string processName) : 
    VariableManager(processName), 
    monitorSize(0), controlSize(0),
    byteCount(0), bitCount(0), previousEntity(""), msgWriteCounter(0)
{
    for(string key: variableMapKeyOrder) {
        VariablePtr var = variableMap[key];
        if (var->getPlcType() == PlcType::MONITOR) {
            plcMonitorVariables.push_back(var);
        } else if (var->getPlcType() == PlcType::CONTROL) {
            plcControlVariables.push_back(var);
        } else if (var->getPlcType() == PlcType::NONE) {
            pcVariables.push_back(var);
        }
    }
}

PlcVariableManager::~PlcVariableManager()
{
    delete[] controlData;
    delete[] monitorData;
}

void PlcVariableManager::resetCount() {
    if (jConfig["protocols"]["plc"]["protocol"] == "s7") {
        byteCount = 0;
    } else if (jConfig["protocols"]["plc"]["protocol"] == "udp") { 
        bitCount = 20;  // account for header bytes
    }
    bitCount = 0;
    previousEntity = "";
}

void PlcVariableManager::beginCount(VariablePtr var) {
    if (jConfig["protocols"]["plc"]["protocol"] == "s7") {
        string newEntity = var->getEntity();
        if (var->getType() != "bool" || newEntity != previousEntity) {
            if (bitCount != 0) {
                bitCount = 0;
                byteCount += 2;
            }
        }
        previousEntity = newEntity;
    } else if (jConfig["protocols"]["plc"]["protocol"] == "udp") { 
        string newEntity = var->getEntity();
        previousEntity = newEntity;
    }

}

void PlcVariableManager::endCount(VariablePtr var) {
    if (jConfig["protocols"]["plc"]["protocol"] == "s7") {
        if (var->getType() == "bool") {
            bitCount += 1;
            if (bitCount == 8) {
                bitCount = 0;
                byteCount += 1;
            } 
        } else {
            bitCount = 0;
            byteCount += var->getSize();
        }
    } else if (jConfig["protocols"]["plc"]["protocol"] == "udp") { 
        bitCount = 0;  // Booleans are full bits
        byteCount += var->getSize();
    }
}

void PlcVariableManager::setSize(PlcType plcType) 
{
    vector<VariablePtr> variables;
    if (plcType == PlcType::MONITOR) {
        variables = plcMonitorVariables;
    } else if (plcType == PlcType::CONTROL) {
        variables = plcControlVariables;
    }

    resetCount();
    for (VariablePtr var: variables) {
        beginCount(var);
        endCount(var);
    }

    if (plcType == PlcType::MONITOR) {
        monitorSize  = byteCount + (bitCount > 0 ? 1 : 0);
    } else if (plcType == PlcType::CONTROL) {
        controlSize  = byteCount + (bitCount > 0 ? 1 : 0);
    }
}

void PlcVariableManager::printRapport(LoggerStream& logger, vector<VariablePtr>& variables)
{
    stringstream s;
    s << endl;
    TablePrinter tp(&s);
    tp.AddColumn("Variable", 10);
    tp.AddColumn("Key", 50);
    tp.AddColumn("Group", 15);
    tp.AddColumn("Entity", 15);
    tp.AddColumn("Type", 10);
    tp.AddColumn("Value", 15);
    tp.AddColumn("Plc byte.bit", 15);

    resetCount();
    tp.PrintHeader();
    int cnt = 0;
    for (VariablePtr var: variables) {
        tp << cnt++;
        beginCount(var);

        string byteBitStr = (var->getPlcType() != PlcType::NONE) ? to_string(byteCount) + "." + to_string(bitCount) : "";

        // fill in variables
        if (var->getType().find("int") != string::npos) {
            tp << var->getName() << var->getGroup() << var->getEntity() << var->getType() << var->getValue<int>() << byteBitStr;
        } else if (var->getType().find("float") != string::npos) {
            tp << var->getName() << var->getGroup() << var->getEntity() << var->getType() << var->getValue<double>() << byteBitStr;
        } else if (var->getType() == "string") {
            tp << var->getName() << var->getGroup() << var->getEntity() << var->getType() << var->getValue<string>() << byteBitStr;
        } else if (var->getType() == "bool") {
            tp << var->getName() << var->getGroup() << var->getEntity() << var->getType() << var->getValue<bool>() << byteBitStr;
        } else {
            throw PlcNoSuchDataTypeException(var);
        }

        endCount(var);
    }

    tp.PrintFooter();
    logger << s.str();
}

void PlcVariableManager::printUdpHeader(LoggerStream& logger, std::vector<uint8_t>& data)
{
    stringstream s;
    s << endl;
    TablePrinter tp(&s);
    tp.AddColumn("ID", 20);
    tp.AddColumn("Hex", 15);
    tp.AddColumn("Dec", 5);

    auto format_bytes = [](const std::vector<uint8_t>& b) -> std::string {
        stringstream ss;
        for (size_t i = 0; i < b.size(); ++i) {
            ss << std::format("{:02X}", b[i]);
            if (i < b.size() - 1) {
                ss << " ";
            }
        }
        return ss.str();
    };

    auto bytes_to_number = [](const std::vector<uint8_t>& b) -> uint16_t {
        return static_cast<uint16_t>(b[0]) |
              (static_cast<uint16_t>(b[1]) << 8);
    };

    tp.PrintHeader();
    
    tp << "NVL UDP";
    tp << format_bytes({data[0], data[1], data[2], data[3]});
    tp << "";

    tp << "Reversed / Flags";
    tp << format_bytes({data[4], data[5], data[6], data[7]});
    tp << "";

    tp << "Publisher ID";
    tp << format_bytes({data[8], data[9]});
    tp << bytes_to_number({data[8], data[9]});

    tp << "Position";
    tp << format_bytes({data[10], data[11]});
    tp << bytes_to_number({data[10], data[11]});

    tp << "Number of variables";
    tp << format_bytes({data[12], data[13]});
    tp << bytes_to_number({data[12], data[13]});

    tp << "Number of bytes";
    tp << format_bytes({data[14], data[15]});
    tp << bytes_to_number({data[14], data[15]});

    tp.PrintFooter();
    logger << s.str();
}

void PlcVariableManager::formatUdpHeader()
{
    // Change number of variable
    uint16_t numVars = plcControlVariables.size();
    unsigned char *numVars_char = reinterpret_cast<unsigned char*>(&numVars);
    udpSendHeader[12] = (unsigned char) (*numVars_char & 0xFF);
    udpSendHeader[13] = (unsigned char) ((*numVars_char >> 8) & 0xFF);

    // Change number of bytes
    uint16_t numBytes = controlSize; 
    unsigned char *numBytes_char = reinterpret_cast<unsigned char*>(&numBytes);
    udpSendHeader[14] = (unsigned char) (*numBytes_char & 0xFF);
    udpSendHeader[15] = (unsigned char) ((*numBytes_char >> 8) & 0xFF);
}

void PlcVariableManager::writeControlValuesToPlc()
{
    // make sure array is empty
    for (int i = 0; i < controlSize; i++) {
        controlData[i] = 0;
    }

    // Add header data
    // The byte sequence for the header + counter (20 bytes total)
    std::memcpy(controlData, udpSendHeader.data(), udpSendHeader.size());

    // Add counter (still part of the header)
    uint32_t val = msgWriteCounter;
    unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
    controlData[16] = (unsigned char) (*val_char & 0xFF);
    controlData[17] = (unsigned char) ((*val_char >> 8) & 0xFF);
    controlData[18] = (unsigned char) ((*val_char >> 16) & 0xFF);
    controlData[19] = (unsigned char) ((*val_char >> 24) & 0xFF);
    // increase message counter
    msgWriteCounter = (msgWriteCounter + 1) % std::numeric_limits<uint32_t>::max();

    // Continue with data
    resetCount();
    for (VariablePtr var: plcControlVariables) {
        beginCount(var);

        // fill in variables
        if (var->getType() == "int8") {
            int8_t val = var->getValue<int>();
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) (*val_char & 0xFF);
        } else if (var->getType() == "uint8") {
            uint8_t val = var->getValue<int>();
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) (*val_char & 0xFF);
        } else if (var->getType() == "int16") {
            int16_t val = var->getValue<int>();
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) (*val_char & 0xFF);
            controlData[byteCount+1] = (unsigned char) ((*val_char >> 8) & 0xFF);
        } else if (var->getType() == "uint16") {
            uint16_t val = var->getValue<int>();
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) (*val_char & 0xFF);
            controlData[byteCount+1] = (unsigned char) ((*val_char >> 8) & 0xFF);
        } else if (var->getType() == "int32") {
            int32_t val = var->getValue<int>();
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) (*val_char & 0xFF);
            controlData[byteCount+1] = (unsigned char) ((*val_char >> 8) & 0xFF);
            controlData[byteCount+2] = (unsigned char) ((*val_char >> 16) & 0xFF);
            controlData[byteCount+3] = (unsigned char) ((*val_char >> 24) & 0xFF);
        } else if (var->getType() == "uint32") {
            uint32_t val = var->getValue<uint>();
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) (*val_char & 0xFF);
            controlData[byteCount+1] = (unsigned char) ((*val_char >> 8) & 0xFF);
            controlData[byteCount+2] = (unsigned char) ((*val_char >> 16) & 0xFF);
            controlData[byteCount+3] = (unsigned char) ((*val_char >> 24) & 0xFF);
        } else if (var->getType() == "float") {
            double value = var->getValue<double>();
            IEEE_754::_2008::Binary<32> val(value);
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) val_char[0];
            controlData[byteCount+1] = (unsigned char) val_char[1];
            controlData[byteCount+2] = (unsigned char) val_char[2];
            controlData[byteCount+3] = (unsigned char) val_char[3];
        } else if (var->getType() == "lfloat") {
            double value = var->getValue<double>();
            IEEE_754::_2008::Binary<64> val(value);
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) val_char[0];
            controlData[byteCount+1] = (unsigned char) val_char[1];
            controlData[byteCount+2] = (unsigned char) val_char[2];
            controlData[byteCount+3] = (unsigned char) val_char[3];
            controlData[byteCount+4] = (unsigned char) val_char[4];
            controlData[byteCount+5] = (unsigned char) val_char[5];
            controlData[byteCount+6] = (unsigned char) val_char[6];
            controlData[byteCount+7] = (unsigned char) val_char[7];
        } else if (var->getType() == "string") {
            string val = var->getValue<string>();
            unsigned char* val_char = reinterpret_cast<unsigned char*>(&val);
            memcpy((void*) (controlData[byteCount]), val_char, 16);  // TODO compile warning on this!
        } else if (var->getType() == "bool") {
            int val = var->getValue<bool>();
            unsigned char *val_char = reinterpret_cast<unsigned char*>(&val);
            controlData[byteCount+0] = (unsigned char) (*val_char & 0xFF);

            if (jConfig["protocols"]["plc"]["protocol"] == "s7") {
                // Counts bools as bits with s7
                unsigned char plcval_curr = controlData[byteCount];
                unsigned char plcval_write = (((*val_char) << bitCount) & 0xFF);
                
                controlData[byteCount+0] = plcval_write | plcval_curr;
            } 
        } else {
            throw PlcNoSuchDataTypeException(var);
        }

        endCount(var);
    }

    // write data to plc
    plcPtr->write(controlSize, controlData);
}

void PlcVariableManager::readMonitorValuesFromPlc()
{
    // read data from plc
    if (monitorSize > 0) {
        plcPtr->read(monitorSize, monitorData);
    }

    // extract read values
    resetCount();
    for (VariablePtr var: plcMonitorVariables) {
        beginCount(var);

        // extract variables
        if (var->getType() == "int8") {
            int8_t val = (int8_t) (monitorData[byteCount+0]);
            var->setValue(val);
        } else if (var->getType() == "uint8") {
            uint8_t val = (uint8_t) (monitorData[byteCount+0]);
            var->setValue(val);
        } else if (var->getType() == "int16") {
            int16_t val = (int16_t) ((monitorData[byteCount+0] << 8) | monitorData[byteCount+1]);
            var->setValue(val);
        } else if (var->getType() == "uint16") {
            uint16_t val = (uint16_t) ((monitorData[byteCount+0] << 8) | monitorData[byteCount+1]);
            var->setValue(val);
        } else if (var->getType() == "int32") {
            int32_t val = (int32_t) ((monitorData[byteCount+0] << 24) | (monitorData[byteCount+1] << 16) | (monitorData[byteCount+2] << 8) | monitorData[byteCount+3]);
            var->setValue(val);
        } else if (var->getType() == "uint32") {
            uint32_t val = (uint32_t) ((monitorData[byteCount+0] << 24) | (monitorData[byteCount+1] << 16) | (monitorData[byteCount+2] << 8) | monitorData[byteCount+3]);
            var->setValue(val);
        } else if (var->getType() == "float") {
            int const plc_size = 4;
            IEEE_754::_2008::Binary<32> f;
            unsigned char b[] = {monitorData[byteCount+0], monitorData[byteCount+1], monitorData[byteCount+2], monitorData[byteCount+3]};
            memcpy(&f, &b, plc_size);
            var->setValue((double) f);
        } else if (var->getType() == "lfloat") {
            int const plc_size = 8;
            IEEE_754::_2008::Binary<64> f;
            unsigned char b[] = {monitorData[byteCount+0], monitorData[byteCount+1], monitorData[byteCount+2], monitorData[byteCount+3], monitorData[byteCount+4], monitorData[byteCount+5], monitorData[byteCount+6], monitorData[byteCount+7]};
            memcpy(&f, &b, plc_size);
            var->setValue((double) f);
        } else if (var->getType() == "string") {
            char val_data[17] = {'\0'};
            memcpy(val_data, monitorData + byteCount, 16);
            string val(val_data);
            var->setValue(trim(val));
        } else if (var->getType() == "bool") {
            if (jConfig["protocols"]["plc"]["protocol"] == "s7") {
                var->setValue((bool) ((monitorData[byteCount+0] >> bitCount) & 0x01));
            } else if (jConfig["protocols"]["plc"]["protocol"] == "udp") {
                var->setValue((bool) (monitorData[byteCount+0] & 0x01));
            }
        } else {
            throw PlcNoSuchDataTypeException(var);
        }

        endCount(var);
    }
}

void PlcVariableManager::init()
{
    LoggerStream::getInstance() << INFO << "## PLC Rapport for the monitorData ##";
    printRapport(LoggerStream::getInstance(), plcMonitorVariables);

    LoggerStream::getInstance() << INFO << "## PLC Rapport for the controlData ##";
    printRapport(LoggerStream::getInstance(), plcControlVariables);

    LoggerStream::getInstance() << INFO << "## PC Rapport ##";
    printRapport(LoggerStream::getInstance(), pcVariables);

    // buffers
    setSize(PlcType::MONITOR);
    setSize(PlcType::CONTROL);
    controlData = new unsigned char[controlSize];
    monitorData = new unsigned char[monitorSize];

    if (jConfig["protocols"]["plc"]["protocol"] == "udp") {
        // Create header for udp send data
        formatUdpHeader();
        LoggerStream::getInstance() << INFO << "## UDP header ##";
        printUdpHeader(LoggerStream::getInstance(), udpSendHeader);
    }

    LoggerStream::getInstance() << INFO << "-- PLC monitorData is " << monitorSize << " bytes and has " << plcMonitorVariables.size() << " variables.";
    LoggerStream::getInstance() << INFO << "-- PLC controlData is " << controlSize << " bytes and has " << plcControlVariables.size() << " variables.";
    LoggerStream::getInstance() << INFO << "-- PC data has " << pcVariables.size() << " variables.";

    // connect to plc
    if (jConfig["protocols"]["plc"]["protocol"] == "s7") {
        plcPtr = make_unique<S7Plc>(jConfig["protocols"]["plc"]);
    } else if (jConfig["protocols"]["plc"]["protocol"] == "udp") {
        plcPtr = make_unique<UdpPlc>(jConfig["protocols"]["plc"]);
    } else {
        LoggerStream::getInstance() << ERROR << "PLC communication type " << jConfig["protocols"]["plc"]["protocol"] << " is not supported. Supported types are 's7' and 'udp'.";
        throw PlcProtocolNotSupportedException(jConfig["protocols"]["plc"]["protocol"]);
    }
}

void PlcVariableManager::serverTick() 
{
    if (plcPtr->connected()) {
        writeControlValuesToPlc();
        readMonitorValuesFromPlc();
    } else {
        throw PlcNotFound(plcPtr->getIp());
    }
}
