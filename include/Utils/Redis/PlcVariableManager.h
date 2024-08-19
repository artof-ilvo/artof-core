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
        void setSize(PlcType plcType);
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

