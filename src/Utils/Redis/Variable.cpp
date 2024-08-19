#include <Utils/Redis/Variable.h>
#include <Utils/Logging/LoggerStream.h>

#include <vector>
#include <iomanip>
#include <cctype>
#include <algorithm>

using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;
using namespace nlohmann;
using namespace std;

namespace Ilvo {
namespace Utils {
namespace Redis {
    std::map<std::string, int> typeSizeMap = {
        {"int8", 1},
        {"uint8", 1},
        {"int16", 2},
        {"uint16", 2},
        {"int32", 4},
        {"uint32", 4},
        {"float", 4},
        {"lfloat", 8},
        {"string", 8},
        {"bool", 1},
        {"int", sizeof(int)},
        {"double", sizeof(double)}
    };
}
}
}

Variable::Variable(string name, string group, string entity, string type, PlcType plcType) : 
   name(name), group(group), entity(entity), type(type), plcType(plcType), updated(false) 
{
}

const string& Variable::getName() const
{
    return name;
}
const string& Variable::getType() const
{
    return type;
}
const string& Variable::getEntity() const
{
    return entity;
}
const string& Variable::getGroup() const
{
    return group;
}
const PlcType& Variable::getPlcType() const
{
    return plcType;
}
const int Variable::getSize()
{
    return typeSizeMap[type];
}

void Variable::setValueString(string valueStr)
{
    bool valueIsNil = valueStr.empty();
    try {
        if (type.find("int") != string::npos) {
            if (valueIsNil) value =  (int) 0;
            else value = stoi(valueStr);
        } else if (type.find("float") != string::npos || type == "double") {
            if (valueIsNil) value = (double) 0.0;
            else value = stod(valueStr);
        } else if (type == "bool") {
            if (valueIsNil) {
                value = false;
            } else {
                transform(valueStr.begin(), valueStr.end(), valueStr.begin(), ::tolower);
                value = (valueStr == "true" || valueStr == "1");
            }
        } else if (type == "string") {
            value = valueStr;
        } else {
            throw Ilvo::Exception::RedisTypeNotFoundException(type);
        }
    } catch(const bad_cast& e) {
        throw RedisVariableBadDefaultValueCastExceptions(name, type);
    } catch (const exception& e) {
        LoggerStream::getInstance() << ERROR << "Unexpected exception for variable \'" << name << "\': " << e.what() << ", value: " << valueStr;
    }
}

void Variable::setDefaultValue() {
    if (type.find("int") != string::npos) {
        value = 0;
    } else if (type.find("float") != string::npos || type == "double") {
        value = 0.0;
    } else if (type == "bool") {
        value = false;
    } else if (type == "string") {
        // Do net set an emtpy string this gives strange errors on the redis stream
        value = "-";
    } else {
        throw Ilvo::Exception::RedisTypeNotFoundException(type);
    }
}

std::string Variable::getValueAsString()
{
    stringstream ss;
    try {
        if (std::holds_alternative<int>(value)) {
            ss << get<int>(value);
        } else if (std::holds_alternative<uint>(value)) {
            ss << get<uint>(value);
        } else if (std::holds_alternative<double>(value)) {
            ss << setprecision(10) << get<double>(value);
        } else if (std::holds_alternative<bool>(value)) {
            ss << (get<bool>(value) ? "true" : "false");
        } else if (std::holds_alternative<std::string>(value)) {
            ss << get<std::string>(value);
        } else {
            throw Ilvo::Exception::RedisTypeNotFoundException(type);
        }
    } catch (const std::bad_variant_access&) {
        throw Ilvo::Exception::RedisVariableBadCastExceptions(getName(), getType());
    }
    return ss.str();
}

bool Variable::isUpdated()
{
    return updated;
}

void Variable::setUpdated(bool updated)
{
    this->updated = updated;
}