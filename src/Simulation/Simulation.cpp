#include <Utils/Geometry/Transform.h>
#include <Utils/Geometry/Angle.h>
#include <Utils/Logging/LoggerStream.h>
#include <Simulation/Simulation.h>
#include <Exceptions/RobotExceptions.hpp>
#include <Exceptions/FileExceptions.hpp>

#include <math.h>
#include <chrono>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;

using namespace std;
using namespace std::chrono_literals;
using namespace nlohmann;
using namespace Eigen;


Simulation::Simulation(const string ns) : 
    VariableManager(ns), discreteImplementActive(false)
{}

void Simulation::init() {
    field = std::make_shared<Field>(Field::checkFieldName(getVariable("pc.field.name")->getValue<string>()), platform.gps.utm_zone);
}

void Simulation::serverTick() {
    // Propagate simulation to programming mode
    getVariable("plc.control.substate.programming")->setValue<bool>(getVariable("pc.simulation.active")->getValue<bool>());

    // Check for end reached
    if (getVariable("plc.control.navigation.end_reached")->getValue<bool>()) {
        LoggerStream::getInstance() << DEBUG <<"set simulationAuto False";
        getVariable("pc.simulation.auto")->setValue<bool>(false);
    }

    // update field
    edgeDetectorField.detect( getVariable("pc.field.updated")->getValue<bool>());
    if ( edgeDetectorField.falling ) {
        field = std::make_shared<Field>(Field::checkFieldName(getVariable("pc.field.name")->getValue<string>()), platform.gps.utm_zone);
    }

    // simulation
    if ( getVariable("pc.simulation.active")->getValue<bool>() ) {
        // ** GET CURRENT STATE **
        // Robot local coordinate system
        //      y
        //      |
        //      |
        //    z .____ x
        double yVelocity = getVariable("plc.control.navigation.velocity.longitudinal")->getValue<double>();
        double xVelocity = 0;
        if (platform.navModesContainsId(AlgorithmMode::PP_SPINNING_180)) {
            xVelocity = getVariable("plc.control.navigation.velocity.lateral")->getValue<double>();
        }
        double zVelocity = getVariable("plc.control.navigation.velocity.angular")->getValue<double>();

        getVariable("plc.monitor.navigation.velocity.longitudinal")->setValue<double>(yVelocity);
        getVariable("plc.monitor.navigation.velocity.lateral")->setValue<double>(xVelocity);
        getVariable("plc.monitor.navigation.velocity.angular")->setValue<double>(zVelocity);

        // ** GET NEW STATE **
        // GET DISCR IMPL DURING PROGRAMMING MODE TEST WITH PLC
        // detect edges
        if (field->hasTasksWithType("discrete")) {
            Task& task = field->getTaskWithType("discrete");

            string hitchName = task.getHitch().getEntityName();
            string activateName = "plc.control." + hitchName + ".activate_discrete";
            string busyName = "plc.monitor." + hitchName + ".busy";
            string notificationName = "pc.execution.notification";

            // detect edges for discrete implementat simulation
            startDiscreteImplementEdge.detect(getVariable(activateName)->getValue<bool>());
            busyDiscrImplEdge.detect(getVariable(busyName)->getValue<bool>());

            if (!discreteImplementActive && startDiscreteImplementEdge.rising) {
                discreteImplementActive = true;
                getVariable(busyName)->setValue<bool>(true);
                if (!getVariable("plc.monitor.state.auto")->getValue<bool>()) {
                    // only when not attachted to actual robot, otherwise wait on robot change of busy variable
                    getVariable(notificationName)->setValue("Setting variable " + busyName + " to false to continue the simulation.");
                }
            }
            if (discreteImplementActive) {
                if (busyDiscrImplEdge.falling) {
                    discreteImplementActive = false;
                } 
                
                // turn of busy when user notification was acknowledged
                if (!getVariable("plc.monitor.state.auto")->getValue<bool>()) {
                    // only when not attachted to actual robot, otherwise wait on robot change of busy variable
                    string notification = getVariable(notificationName)->getValue<string>();
                    notificationAcknowledgeEdge.detect(notification == "-");
                    if (notificationAcknowledgeEdge.rising) {
                        getVariable(busyName)->setValue(false);
                    }
                }

            }

        }


        // CREATE NEW STATE VARIABLES
        if (!(clk.checkTimerBusy() || discreteImplementActive)) {
            platform.robot.updateState(getRedisState("robot.ref").asAffine());

            double ts = clk.getIntervalMs()*1e-3 * getVariable("pc.simulation.factor")->getValue<double>(); // 50 ms
            
            Affine3d velTransform = vectorToAffine(Vector3d(xVelocity * ts, yVelocity * ts, 0.0), Vector3d(0.0, 0.0, zVelocity * ts), false);
            State newRawGpsState = platform.applyVelocityOnRobotRef(velTransform);

            // ** SET NEW STATE **
            setRedisJsonStates(platform, newRawGpsState);  
        } 
    } 
}

int main() {
    // First check if ILVO_PATH environment variable is set
    if (getenv("ILVO_PATH") == NULL) { 
        throw EnvVariableNotFoundException("$ILVO_PATH");
    }

    string procName = "ilvo-simulation";
    LoggerStream::createInstance(procName);
    LoggerStream::getInstance() << INFO << "Simulation Logger started";
    Simulation simulation(procName);
    simulation.run();
    return 0;    
}