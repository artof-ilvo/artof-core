/**
 * @file Variable.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Abstraction of a redis variable in the system
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <vector>
#include <variant>
#include <ThirdParty/json.hpp>
#include <Utils/Timing/Clk.h>
#include <Utils/String/String.h>
#include <cstdlib>
#include <iostream>
#include <Exceptions/RedisExceptions.hpp>
#include <stdexcept>

namespace Ilvo {
namespace Utils {
namespace Redis {

    enum PlcType {MONITOR, CONTROL, NONE};

    extern std::map<std::string, int> typeSizeMap;

    /**
     * @brief A redis variable in the system
     * 
     * @details A variable is a redis variable in the system
     */
    class Variable
    {
    private:
        std::string name;
        std::string type;
        std::string entity;
        std::string group;
        PlcType plcType;
        bool updated;
        std::variant<double, bool, int, uint, std::string> value;
    public:
        Variable(std::string name, std::string group, std::string entity, std::string type, PlcType plcType);
        virtual ~Variable() = default;

        const std::string& getName() const;
        const std::string& getType() const;
        const std::string& getEntity() const;
        const std::string& getGroup() const;
        const PlcType& getPlcType() const;
        const int getSize();

        bool isUpdated();
        void setUpdated(bool updated);

        void setValueString(std::string valueStr);
        void setDefaultValue();
        std::string getValueAsString();

        template <typename T>
        void setValue(T value)
        {
            setValueString(Utils::String::toRedisString<T>(value));
            updated = true;
        }

        template <typename T>
        T getValue()
        {
            if constexpr (std::is_same<T, bool>::value) {
                if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
                return false;
            } else if constexpr (std::is_same<T, std::string>::value) {
                if (std::holds_alternative<std::string>(value)) return std::get<std::string>(value);
                return "";
            } else {
                if (std::holds_alternative<T>(value)) return std::get<T>(value);
                return 0;
            }
        }
    };

    inline std::ostream& operator<<(std::ostream & os, const Variable& variable)
    {
        os << "(" << variable.getName() << ") [" << variable.getType() << "]";
        return os;
    }

    typedef std::shared_ptr<Variable> VariablePtr;

} // Redis
} // Utils
} // Ilvo



