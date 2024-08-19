/**
 * @file Plc.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Abstraction layer for a Siemens PLC (Snap7)
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <ThirdParty/json.hpp>
#include <ThirdParty/ieee754_types.hpp>
#include <ThirdParty/snap7/snap7.h>

namespace Ilvo {
namespace Utils {
namespace Redis {

    class Plc: public TS7Client
    {        
    public:
        /** @brief The datablock in the PLC to read */
        int readDb;
        /** @brief The datablock in the PLC to write */
        int writeDb;

        /** @brief The IP address of the PLC */
        std::string ip;
        /** @brief The rack of the PLC */
        uint8_t rack;
        /** @brief The slot of the PLC */
        uint8_t slot;

        Plc() = default;
        /** @brief Construct a new Plc object from the json configuration */
        Plc(nlohmann::ordered_json& j);
        ~Plc();
    };

} // Redis
} // Utils
} // Ilvo