#include <Utils/Redis/PlcVariableManager.h>
#include <Utils/String/String.h>
#include <ThirdParty/snap7/snap7.h>
#include <ThirdParty/ieee754_types.hpp>
#include <cstddef>
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
    byteCount = 20;  // account for udp header
    bitCount = 0;
    previousEntity = "";
}

void PlcVariableManager::beginCount(VariablePtr var) {
    string newEntity = var->getEntity();
    if (var->getType() != "bool" || newEntity != previousEntity) {
        if (bitCount != 0) {
            bitCount = 0;
            byteCount += 2;
        }
    }
    previousEntity = newEntity;
}

void PlcVariableManager::endCount(VariablePtr var) {
    if (var->getType() == "bool") {
        // bitCount += 1;
        // if (bitCount == 8) {
        //     bitCount = 0;
        //     byteCount += 1;
        // } 
        byteCount += 1;
    } else {
        bitCount = 0;
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

    monitorSize += 20; // account for udp header
    controlSize += 20; // account for udp header
}

void PlcVariableManager::printRapport(LoggerStream& logger, vector<VariablePtr>& variables)
{
    stringstream s;
    s << endl;
    TablePrinter tp(&s);
    tp.AddColumn("Key", 50);
    tp.AddColumn("Group", 15);
    tp.AddColumn("Entity", 15);
    tp.AddColumn("Type", 10);
    tp.AddColumn("Value", 15);
    tp.AddColumn("Plc byte.bit", 15);

    resetCount();
    tp.PrintHeader();
    for (VariablePtr var: variables) {
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

void PlcVariableManager::writeControlValuesToPlc()
{
    // make sure array is empty
    for (int i = 0; i < controlSize; i++) {
        controlData[i] = 0;
    }

    // Add header data
    // The byte sequence for the header (20 bytes total)
    std::vector<uint8_t> header_bytes = {
        // NVL UDP identifier (4 bytes)
        0x00, 0x2d, 0x53, 0x33, 
        
        // reserved / flags (4 bytes)
        0x00, 0x00, 0x00, 0x00, 
        
        // publisher ID = 2 (2 bytes - Little Endian or Network Byte Order 02 00? Assuming Little Endian from input: 02 00)
        0x05, 0x00, 
        
        // position = 0 (2 bytes)
        0x00, 0x00,
        
        // number of variables = 1 (2 bytes)
        // TODO add the number of variables as a variable
        0x31, 0x00, 
        
        // length = 82 bytes (header 20 + data [data size]) (2 bytes - Little Endian or Network Byte Order 18 00? Assuming Little Endian from input: 18 00)
        // TODO add the number of bytes as a variable
        0x59, 0x00
        
        // Note: The total bytes added is 4 + 4 + 2 + 2 + 2 + 2 = 16 bytes. 
        // Let's check the input again:
        // "00 2d 53 33 " - 4 bytes
        // "00 00 00 00 " - 4 bytes
        // "02 00 " - 2 bytes
        // "00 00 " - 2 bytes
        // "01 00 " - 2 bytes
        // "18 00" - 2 bytes
        // TOTAL = 16 bytes.
        
        // **Correction based on common protocol headers:** // Your input string seems to be missing 4 bytes to reach the common 20-byte header length. 
        // Based on the provided hex stream, the total is 16 bytes. 
        // I will use the 16 bytes provided.
        // If the header *should* be 20 bytes, you'd need to add two more 2-byte fields (4 bytes total).
    };
    std::memcpy(controlData, header_bytes.data(), header_bytes.size());

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

            // unsigned char plcval_curr = controlData[byteCount];
            // unsigned char plcval_write = (((*val_char) << bitCount) & 0xFF);
            
            // controlData[byteCount+0] = plcval_write | plcval_curr;
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
            var->setValue((bool) (monitorData[byteCount+0] & 0x01));
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
    LoggerStream::getInstance() << INFO;
    LoggerStream::getInstance() << INFO << "## PLC Rapport for the controlData ##";
    printRapport(LoggerStream::getInstance(), plcControlVariables);
    LoggerStream::getInstance() << INFO;
    LoggerStream::getInstance() << INFO << "## PC Rapport ##";
    printRapport(LoggerStream::getInstance(), pcVariables);
    LoggerStream::getInstance() << INFO;

    // buffers
    setSize(PlcType::MONITOR);
    setSize(PlcType::CONTROL);
    LoggerStream::getInstance() << INFO << "-- monitorData has size: " << monitorSize << " bytes.";
    LoggerStream::getInstance() << INFO << "-- controlData has size: " << controlSize << " bytes.";
    controlData = new unsigned char[controlSize];
    monitorData = new unsigned char[monitorSize];

    // load data arrays


    // connect to plc
    if (jConfig["protocols"]["plc"]["protocol"] == "s7") {
        plcPtr = make_unique<S7Plc>(jConfig["protocols"]["plc"]);
    } else if (jConfig["protocols"]["plc"]["protocol"] == "udp") {
        plcPtr = make_unique<UdpPlc>(jConfig["protocols"]["plc"]);
    } else {
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
