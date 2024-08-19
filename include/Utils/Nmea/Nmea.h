/**
 * @file Nmea.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief NMEA parsing functions
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <map>

#include <Exceptions/NmeaExceptions.hpp>

namespace Ilvo {
namespace Utils {
namespace Nmea {

    /** @brief Supported NMEA message types */
    enum NmeaMessageType { GGA, HDT, VTG, HRP };
    /** @brief Supported NMEA field types */
    enum NmeaFieldType { INT, FLOAT, LONG, STRING, DOUBLE };

    /** @brief NMEA field definition */
    class NmeaField {
        public:
            std::string name;
            NmeaFieldType type;
            int idx;

        public:
            NmeaField(std::string name, NmeaFieldType type, int idx): name(name), type(type), idx(idx) {};
            NmeaField(NmeaField* field) : name(field->name), type(field->type), idx(field->idx) {};
    };

    class NmeaFieldValue : public NmeaField {
        public:
            union {
                int i;
                float f;
                double d;
                long l;
                std::string s;
            };

            NmeaFieldValue(NmeaField* field, int i): NmeaField(field), i(i) {}; 
            NmeaFieldValue(NmeaField* field, float f): NmeaField(field), f(f) {}; 
            NmeaFieldValue(NmeaField* field, double d): NmeaField(field), d(d) {}; 
            NmeaFieldValue(NmeaField* field, long l): NmeaField(field), l(l) {}; 
            NmeaFieldValue(NmeaField* field, std::string s): NmeaField(field), s(s) {};
            ~NmeaFieldValue() {};
    };
    typedef std::shared_ptr<NmeaFieldValue> NmeaFieldValuePtr;

    /** @brief Parser of single NMEA line */
    class NmeaLine
    {
        private:
            std::string nmeaLineStr;
            NmeaMessageType type;

            bool nmeaChecksum();
            void parse();                
            std::map<std::string, NmeaFieldValuePtr> fieldValues;
        public:
            NmeaLine() = default;
            NmeaLine(std::vector<char>& nmeaLine);
            NmeaLine(NmeaLine& nmeaLine);
            ~NmeaLine() = default;

            std::map<std::string, NmeaFieldValuePtr>& getFieldValues();            
            std::string& str();
            NmeaMessageType getType();
            bool ok();

            template <typename T>
            T getValue(std::string name) {
                T value;
                NmeaFieldValuePtr ptr = nullptr;

                try {
                    NmeaFieldValuePtr ptr = fieldValues.at(name);
                    if (ptr->type == NmeaFieldType::INT) {
                        value = ptr->i;
                    } else if (ptr->type == NmeaFieldType::FLOAT) {
                        value = ptr->f;
                    } else if (ptr->type == NmeaFieldType::LONG) {
                        value = ptr->l;
                    } else if (ptr->type == NmeaFieldType::DOUBLE) {
                        value = ptr->d;
                    } else if (ptr->type == NmeaFieldType::STRING) {
                        if constexpr (std::is_same<T, std::string>::value) {
                            if (ptr != nullptr) value = ptr->s;
                        }
                    }
                } catch(const std::out_of_range& e) {
                    throw Ilvo::Exception::NmeaException(name);
                }    

                return value;
            }
    };

} // namespace Nmea
} // namespace Utils
} // namespace Ilvo

std::ostream& operator<<(std::ostream& out, Ilvo::Utils::Nmea::NmeaLine& line);
