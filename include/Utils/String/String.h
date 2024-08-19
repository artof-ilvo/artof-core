/**
 * @file String.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Extended string functionality
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <cstdint>

#include <Exceptions/RedisExceptions.hpp>

namespace Ilvo {
namespace Utils {
namespace String {

    std::vector<int> getIndexList(std::string msg, char s);
    std::string toUpperCase(const std::string& s);
    std::string toLowerCase(const std::string& s);
    std::string trim(std::string& in);

    template<class T>
    inline std::string toRedisString(T value) {
        std::string valueStr = "";
        if constexpr (std::is_same<T, bool>::value) {
            valueStr = value ? "true" : "false";
        } else if constexpr (std::is_same<T, std::string>::value) {
            valueStr = value;
        } else if constexpr (std::is_same<T, const char*>::value) {
            valueStr = std::string(value);
        } else if constexpr (std::is_same<T, int8_t>::value ||
                            std::is_same<T, uint8_t>::value ||
                            std::is_same<T, int16_t>::value ||
                            std::is_same<T, uint16_t>::value ||  
                            std::is_same<T, int32_t>::value || 
                            std::is_same<T, uint32_t>::value || 
                            std::is_same<T, int64_t>::value || 
                            std::is_same<T, uint64_t>::value ||
                            std::is_same<T, int>::value ||
                            std::is_same<T, long>::value ||
                            std::is_same<T, double>::value ||
                            std::is_same<T, float>::value) {    
            valueStr = std::to_string(value);
        } else {
            throw Exception::RedisTypeNotFoundException(typeid(T).name());
        }
        return valueStr;
    }

    std::string getHeartbeatVariableName(std::string ilvoProcessName="");

} // namespace Ilvo
} // namespace Utils
} // namespace Other
