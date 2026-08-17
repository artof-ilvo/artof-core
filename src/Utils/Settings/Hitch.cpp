#include <Utils/Settings/Hitch.h>
#include <Utils/Geometry/Transform.h>
#include <Exceptions/RobotExceptions.hpp>
#include <ThirdParty/Eigen/Geometry>
#include <Utils/Logging/LoggerStream.h>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::String;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Timing;
using namespace Ilvo::Utils::Logging;
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


void Hitch::setActivate(VariableManager* manager, bool activate) { 
    this->active = activate; 
    edgeDetectorActivate.detect(activate);
    string variableName = "plc.control." + getEntityName() + ".activate";
    manager->getVariable(variableName)->setValue(activate);

    // Hitch moving detection
    if (hitchMoving) {
        updateHeight(manager);
        if (abs(height - requestedHeight) <= 5.0) {
            hitchMoving = false;
            LoggerStream::getInstance() << DEBUG <<"Hitch " << getEntityName() << " has stopped moving, height: " << height << ", requestedHeight: " << requestedHeight;
        }
    } else {
        if (edgeDetectorActivate.rising) {
            hitchMoving = true;
            requestedHeight = updateSetpoint(manager);
            LoggerStream::getInstance() << DEBUG <<"Hitch " << getEntityName() << " has started moving, height: " << height << ", requestedHeight: " << requestedHeight;
        } else if (edgeDetectorActivate.falling) {
            hitchMoving = true;
            requestedHeight = 0.0;  // TODO: zo of omgekeerd?
        }
    }
}

void Hitch::setActivateDiscrete(VariableManager* manager,bool activate) { 
    this->activate_discrete = activate; 
    string variableName = "plc.control." + getEntityName() + ".activate_discrete";
    manager->getVariable(variableName)->setValue(activate);
}

void Hitch::setActivateCardan(VariableManager* manager,bool activate) { 
    this->activate_cardan = activate; 
    string variableName = "plc.control." + getEntityName() + ".activate_cardan";
    manager->getVariable(variableName)->setValue(activate);
}

void Hitch::setActivateContinuous(VariableManager* manager,bool activate) { 
    this->activate_continuous = activate; 
    string variableName = "plc.control." + getEntityName() + ".activate_continuous";
    manager->getVariable(variableName)->setValue(activate);
}

void Hitch::setBusy(VariableManager* manager, bool busy) { 
    this->busy = busy; 
    string variableName = "plc.monitor." + getEntityName() + ".busy";
    manager->getVariable(variableName)->setValue(busy);
}

bool Hitch::updateActivate(VariableManager* manager) { 
    string variableName = "plc.control." + getEntityName() + ".activate";
    active = manager->existsVariable(variableName) ? manager->getVariable(variableName)->getValue<bool>() : false;
    return getActive();
}

bool Hitch::updateActivateDiscrete(VariableManager* manager) { 
    string variableName = "plc.control." + getEntityName() + ".activate_discrete";
    activate_discrete = manager->existsVariable(variableName) ? manager->getVariable(variableName)->getValue<bool>() : false;
    return getActivateDiscrete();
}

bool Hitch::updateBusy(VariableManager* manager) { 
    string variableName = "plc.monitor." + getEntityName() + ".busy";
    busy = manager->existsVariable(variableName) ? manager->getVariable(variableName)->getValue<bool>() : false;
    return getBusy();
}

int Hitch::updateSetpoint(VariableManager* manager) { 
    string variableName = "plc.control." + getEntityName() + ".setpoint";
    setpoint = manager->existsVariable(variableName) ? manager->getVariable(variableName)->getValue<int>() : 0;
    return getSetpoint();
}

double Hitch::updateHeight(VariableManager* manager) { 
    string variableName = "plc.monitor." + getEntityName() + ".height";
    height = manager->existsVariable(variableName) ? manager->getVariable(variableName)->getValue<double>() : 0.0;
    return getHeight();
}

double Hitch::updateAngle(VariableManager* manager) { 
    string variableName = "plc.control." + getEntityName() + ".angle";
    angle = manager->existsVariable(variableName) ? manager->getVariable(variableName)->getValue<double>() : 0.0;
    return getAngle();
}

bool Hitch::getActive() { 
    return active; 
}

bool Hitch::getActivateDiscrete() { 
    return activate_discrete; 
}

bool Hitch::getActivateCardan() { 
    return activate_cardan; 
}

bool Hitch::getActivateContinuous() { 
    return activate_continuous; 
}

bool Hitch::getBusy() { 
    return busy; 
}

int Hitch::getSetpoint() { 
    return setpoint; 
}

double Hitch::getHeight() { 
    return height; 
}

double Hitch::getAngle() { 
    return angle; 
}

bool Hitch::getHitchMoving() {
    return hitchMoving;
}

