#include <Utils/Settings/Hitch.h>
#include <Utils/Geometry/Transform.h>
#include <Exceptions/RobotExceptions.hpp>
#include <ThirdParty/Eigen/Geometry>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::String;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Exception;
using namespace nlohmann;
using namespace Eigen;
using namespace std;

Hitch::Hitch() : StateFull() 
{}

Hitch::Hitch(json j) : 
    StateFull(j["transform"])
{
    if (j.contains("id")) id = j["id"];
    else throw SettingsParamNotFoundException("hitch", "id");
    if (j.contains("name")) name = j["name"].get<string>();
    else throw SettingsParamNotFoundException("hitch", "name"); 
    if (j.contains("min")) min = j["min"];
    else throw SettingsParamNotFoundException("hitch", "min"); 
    if (j.contains("max")) max = j["max"];
    else throw SettingsParamNotFoundException("hitch", "max"); 
    if (j.contains("link_length")) link_length = j["link_length"];
    else throw SettingsParamNotFoundException("hitch", "link_length");

    if (j.contains("types")) {
        for (std::string type: j["types"]) types.push_back(type);
    } else {
        throw SettingsParamNotFoundException("hitch", "types");
    }     
}

Affine3d Hitch::getBallState(double angle)
{
    return currentState.asAffine() * calculateHingeTransform(link_length, angle);
}

string Hitch::getEntityName()
{
    return "hitch_" + toLowerCase(name);
}

json Hitch::prepareJson() const {
    json j;
    j["id"] = id;
    j["name"] = name;
    j["min"] = min;
    j["max"] = max;
    j["types"] = types;
    j["link_length"] = link_length;
    return j;
}

json Hitch::toStateFullJson(double angle, int zone)
{
    json j = toJson();
    j["state"]["ref"] = currentState.toJson(zone);
    j["state"]["ball"] = State(getBallState(angle)).toJson(zone);
    return j;
}
