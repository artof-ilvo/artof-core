/**
 * @file PlcVariableManager.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Variable manager for a PLC, keeping variables in sync (plc <-> pc)
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Redis/VariableManager.h>
#include <Utils/Redis/Plc.h>
#include <Utils/Logging/LoggerStream.h>

namespace Ilvo {
namespace Utils {
namespace Redis {
    
    /** @brief Variable manager for a PLC, keeping variables in sync (plc <-> pc) */
    class PlcVariableManager: public VariableManager
    {
    private:
        // Data containers
        /** @brief Buffer for monitor data, calculated at the start */
        int monitorSize;
        /** @brief Buffer for monitor data, calculated at the start */
        int controlSize;
        /** @brief Buffer for monitor data */
        unsigned char *monitorData;
        /** @brief Buffer for control data */
        unsigned char *controlData;

        int32_t msgWriteCounter;

        // Count information
        int byteCount;
        int bitCount;
        std::string previousEntity;

        void resetCount();
        void beginCount(VariablePtr var);
        void endCount(VariablePtr var);

        // Plc
        std::unique_ptr<Plc> plcPtr;

        /** @brief Variables to monitor in the plc */
        std::vector<VariablePtr> plcMonitorVariables;
        /** @brief Variables to control in the plc */
        std::vector<VariablePtr> plcControlVariables;
        /** @brief Remaining variables (only in the pc) */
        std::vector<VariablePtr> pcVariables;

        /** @brief Summarize all variables and their bit and byte positions in the plc */
        void printRapport(Utils::Logging::LoggerStream& logger, std::vector<VariablePtr>& variables);
        void printUdpHeader(Utils::Logging::LoggerStream& logger, std::vector<uint8_t>& data);
        void setSize(PlcType plcType);

        std::vector<uint8_t> udpSendHeader = {
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
            0x61, 0x00
            
        };

        void formatUdpHeader();

    public:
        PlcVariableManager(std::string processName);
        ~PlcVariableManager();

        /** @brief Write the control variables to the plc (these could be updated by the pc) */
        void writeControlValuesToPlc();
        /** @brief Read the monitor variables from the plc (these could be updated by the PLC) */
        void readMonitorValuesFromPlc();

        void init() override;
        void serverTick() override;
    };

} // Redis
} // Utils
} // Ilvo

