#include <Operation/Operation.h>
#include <boost/filesystem.hpp>
#include <ThirdParty/json.hpp>
#include <Utils/Settings/Field.h>
#include <Utils/Timing/Timing.h>
#include <Exceptions/FileExceptions.hpp>
#include <Exceptions/RobotExceptions.hpp>

using namespace Ilvo::Exception;
using namespace Ilvo::Core;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Logging;

using namespace std;
using namespace chrono_literals;
using namespace nlohmann;
using namespace Eigen;
using namespace boost::filesystem;


Operation::Operation(const string ns) : 
    VariableManager(ns)
{ 
}

void Operation::init()
{
    LoggerStream::getInstance() << DEBUG << "Initialize Operation.";
    traject = make_unique<Traject>();
    position = make_unique<PositionData>();
    implementControl.init(this, traject, position);
}


void Operation::setRedisJsonImplStates()
{
    json jImplements = json();
    for (Task& task: traject->getField().getTasks()) {
        bool implementType = find(implementTypes.begin(), implementTypes.end(), task.getType()) != implementTypes.end();
        if (implementType) {
            jImplements[task.getImplement().getName()] = task.getImplement().visualizeJson(platform.gps.utm_zone);
        }   
    }
    rs.setRedisJsonValue("implement.states", jImplements);   
}


void Operation::serverTick() 
{
    updatePlatformState();

    edgeDetectorField.detect( getVariable("pc.field.updated")->getValue<bool>());
    bool activeAuto = getVariable("pc.simulation.auto")->getValue<bool>() || getVariable("plc.monitor.state.auto")->getValue<bool>();
    edgeDetectorAutomode.detect( activeAuto );

    if (edgeDetectorField.rising || traject->empty()) {
        traject->load(Field::checkFieldName(getVariable("pc.field.name")->getValue<string>()), 
                    getPlatform().gps.utm_zone, 
                    getVariable("pc.navigation.spin_angle")->getValue<double>(),
                    getVariable("pc.purepursuit.inter_point_distance")->getValue<double>(),
                    getVariable("pc.navigation.turning_radius")->getValue<double>());

        if (traject->empty()) {
            LoggerStream::getInstance() << DEBUG << "Traject loaded failed";
        } else {
            LoggerStream::getInstance() << DEBUG << "Traject loaded successfully";
        }
    }

    // detect creation or loading of new field.
    if (edgeDetectorAutomode.rising || edgeDetectorField.rising) {
        LoggerStream::getInstance() << INFO <<"Resetting the field: ";
        if (edgeDetectorAutomode.rising) {
            LoggerStream::getInstance() << INFO <<"Auto mode is started.";
        } else if (edgeDetectorField.rising) {
        }
        
        implementControl.reset();
    }

    // update implement control (used in the navigation control)
    implementControl.update(edgeDetectorAutomode.getValue());

    // set redis states
    setRedisJsonImplStates();
}


int main() {
    // First check if ILVO_PATH environment variable is set
    if (getenv("ILVO_PATH") == NULL) { 
        throw EnvVariableNotFoundException("$ILVO_PATH");
    }

    string procName = "ilvo-operation";
    LoggerStream::createInstance(procName);
    LoggerStream::getInstance() << INFO << "Operation Logger started";
    Operation operation(procName);
    operation.run();
    return 0;  
}
