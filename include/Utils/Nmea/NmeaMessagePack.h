/**
 * @file NmeaMessagePack.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Budle NMEA messages that were sent together
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Nmea/Nmea.h>
#include <vector>

namespace Ilvo {
namespace Utils {
namespace Nmea {
    class NmeaMessagePack
    {
    public:
        std::shared_ptr<NmeaLine> gga;
        std::shared_ptr<NmeaLine> vtg;
        std::shared_ptr<NmeaLine> hrp;
        std::shared_ptr<NmeaLine> hdt;

        bool updated_gga = false;
        bool updated_vtg = false;
        bool updated_hrp = false;
        bool updated_hdt = false;

        NmeaMessagePack() = default;
        ~NmeaMessagePack() = default;
        NmeaMessagePack(const NmeaMessagePack& other);

        void addNmeaLine(std::shared_ptr<NmeaLine> line);
        bool isUpdated();
        void reset();
        
    };
    
}
}
}