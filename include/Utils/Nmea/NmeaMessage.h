/**
 * @file NmeaMessage.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Information about a NMEA messages
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

    // GGA
    static NmeaField GGA_ID("id", NmeaFieldType::STRING, 0);
    static NmeaField GGA_TIME("time", NmeaFieldType::DOUBLE, 1);
    static NmeaField GGA_LAT("lat", NmeaFieldType::DOUBLE, 2);
    static NmeaField GGA_LAT_IDX("lat_idx", NmeaFieldType::STRING, 3);
    static NmeaField GGA_LON("lon", NmeaFieldType::DOUBLE, 4);
    static NmeaField GGA_LON_IDX("lon_idx", NmeaFieldType::STRING, 5);
    static NmeaField GGA_FIX("fix", NmeaFieldType::INT, 6);
    static NmeaField GGA_HEIGHT("height", NmeaFieldType::DOUBLE, 9);

    static std::vector<NmeaField*> GGA_MESSAGE_FORMAT {
        &GGA_ID,
        &GGA_TIME,
        &GGA_LAT,
        &GGA_LAT_IDX,
        &GGA_LON,
        &GGA_LON_IDX,
        &GGA_FIX,
        &GGA_HEIGHT
    };

    // HDT
    static NmeaField HDT_ID("id", NmeaFieldType::STRING, 0);
    static NmeaField HDT_HEADING("heading", NmeaFieldType::DOUBLE, 1);

    static std::vector<NmeaField*> HDT_MESSAGE_FORMAT {
        &HDT_ID,
        &HDT_HEADING
    };

    // VTG
    static NmeaField VTG_ID("id", NmeaFieldType::DOUBLE, 0);
    static NmeaField VTG_TRUE_COURSE("true_course", NmeaFieldType::DOUBLE, 1);
    static NmeaField VTG_MAGN_COURSE("magn_course", NmeaFieldType::DOUBLE, 3);
    static NmeaField VTG_GROUND_SPEED_KM_PER_H("ground_speed_km_per_h", NmeaFieldType::DOUBLE, 7);

    static std::vector<NmeaField*> VTG_MESSAGE_FORMAT {
        &VTG_ID,
        &VTG_TRUE_COURSE,
        &VTG_MAGN_COURSE,
        &VTG_GROUND_SPEED_KM_PER_H
    };

    // HRP
    static NmeaField HRP_ID("id", NmeaFieldType::STRING, 1);
    static NmeaField HRP_TIME("time", NmeaFieldType::DOUBLE, 2);
    static NmeaField HRP_DATE("date", NmeaFieldType::DOUBLE, 3);
    static NmeaField HRP_HEADING("heading", NmeaFieldType::DOUBLE, 4);
    static NmeaField HRP_ROLL("roll", NmeaFieldType::DOUBLE, 5);
    static NmeaField HRP_PITCH("pitch", NmeaFieldType::DOUBLE, 6);
    static NmeaField HRP_HEADING_DEVIATION("heading_deviation", NmeaFieldType::DOUBLE, 7);
    static NmeaField HRP_ROLL_DEVIATION("roll_deviation", NmeaFieldType::DOUBLE, 8);
    static NmeaField HRP_PITCH_DEVIATION("pitch_deviation", NmeaFieldType::DOUBLE, 9);
    static NmeaField HRP_NUM_SATTELITES("num_sattelites", NmeaFieldType::DOUBLE, 10);
    static NmeaField HRP_MODE("mode", NmeaFieldType::INT, 11);

    static std::vector<NmeaField*> HRP_MESSAGE_FORMAT {
        &HRP_ID,
        &HRP_TIME,
        &HRP_DATE,
        &HRP_HEADING,
        &HRP_ROLL,
        &HRP_PITCH,
        &HRP_HEADING_DEVIATION,
        &HRP_ROLL_DEVIATION,
        &HRP_PITCH_DEVIATION,
        &HRP_NUM_SATTELITES,
        &HRP_MODE
    };

} // namespace Nmea
} // namespace Utils
} // namespace Ilvo
