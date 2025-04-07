#include <Navigation/Navigation.h>
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


Navigation::Navigation(const string ns) : 
    VariableManager(ns),
    autoModeReset(false),
    autoModeError(false)
{ 
}

void Navigation::init()
{
    LoggerStream::getInstance() << DEBUG << "Initialize Navigation.";
    traject = make_unique<Traject>();
    position = make_unique<PositionData>();
    navigationControl.init(this, traject, position);
}

void Navigation::updatePosition()
{
    updatePlatformState();
    // update robot state
    position->robotRefState = platform.robot.getState();
    position->currentPoint = position->robotRefState.getT();
    position->heading = position->robotRefState.getR().asVector()[2];
    position->robotHeadState = platform.robot.getHeadState();

    if (algorithmMode == AlgorithmMode::EXTERNAL) {
        // set path related parameters to zero
        getVariable("pc.path.orientation")->setValue<double>(0.0);
        getVariable("pc.path.orientation_error")->setValue<double>(0.0);
        getVariable("pc.path.distance_error")->setValue<double>(0.0);
    } else {
        // update path related parameters
        // update closest point
        double distanceToClosestPoint = position->currentPoint.distance(position->closestPoint);
        if (distanceToClosestPoint < position->resetPathDistance) {
            // if the robot is close to the path only update the closest point close to the current point
            int indexdiff = min(traject->interpolationLength() - position->closestPoint.index, int(1.5 / getVariable("pc.purepursuit.inter_point_distance")->getValue<double>()));
            position->closestPoint = traject->closestPoint(position->currentPoint, position->closestPoint.index, position->closestPoint.index+indexdiff);
        } else {
            // update the closest point in the whole path, also update the corners
            position->closestPoint = traject->closestPoint(Point(position->robotRefState.getT()));
            position->corners = traject->cornersByPathIndex(position->closestPoint.index);
        }
        
        // set the orientaiton and deviation parameters of the path
        double pathOrientation = toRobotFrame(traject->absPathOrientation(position->closestPoint.index));
        getVariable("pc.path.orientation")->setValue<double>(pathOrientation);
        Line line = traject->pathLine(position->closestPoint.index);
        getVariable("pc.path.distance_error")->setValue<double>(traject->isPointLeft(position->closestPoint.index, position->currentPoint) * line.distance(position->currentPoint));

        double robotOrientation = position->robotRefState.getR().asVector()[2];
        double orientationError = calcSmallestAngle(robotOrientation, pathOrientation);
        getVariable("pc.path.orientation_error")->setValue<double>(orientationError);
    }
}

void Navigation::resetPosition() 
{
    if (traject->empty()) {
        throw TrajectLengthIsZero();
    }

    LoggerStream::getInstance() << INFO << "Reset position data";
    // The closest point will be the start indexInPath
    double carrotDistance = getVariable("pc.purepursuit.carrot_distance")->getValue<double>();
    double interpolationDistance = getVariable("pc.purepursuit.inter_point_distance")->getValue<double>();
    position->currentPoint = position->robotRefState.getT();
    position->carrotPoint = traject->closestPoint(position->currentPoint, position->closestPoint.index, position->closestPoint.index + (int)(carrotDistance / interpolationDistance) + 10, carrotDistance);
    position->closestPoint = traject->closestPoint(position->currentPoint);
    position->corners = traject->cornersByPathIndex(position->closestPoint.index);
    LoggerStream::getInstance() << DEBUG << "Closest point index: " << position->closestPoint.index;
    LoggerStream::getInstance() << DEBUG << "Carrot point index: " << position->carrotPoint.index;
    LoggerStream::getInstance() << DEBUG << "Previous corner: " << position->corners.previousCorner.cornerIndex << ", Next corner: " << position->corners.nextCorner.cornerIndex;
    position->resetPathDistance = RESET_PATH_DISTANCE_DEFAULT;

    getVariable("pc.implement.disable")->setValue(false);  // Ensure that implement operation is enabled when starting
}


void Navigation::serverTick() 
{
    // update redis operation
    getVariable("plc.control.navigation.heartbeat")->setValue<bool>(heartbeatPulse.getValue());

    // check if hitch is busy
    bool hitchBusy = false;
    for (auto &hitch : platform.hitches) {
        hitchBusy |= getVariable("plc.monitor." + hitch.getEntityName() + ".busy")->getValue<bool>();
    }

    // detect edges
    bool activeAuto = getVariable("pc.simulation.auto")->getValue<bool>() || 
        getVariable("plc.monitor.state.auto")->getValue<bool>() || 
        getVariable("plc.monitor.state.aware")->getValue<bool>() ||
        getVariable("plc.monitor.state.steer")->getValue<bool>() || 
        getVariable("plc.monitor.state.throttle")->getValue<bool>();
    edgeDetectorField.detect( getVariable("pc.field.updated")->getValue<bool>());
    edgeDetectorAutomode.detect( activeAuto );

    // reset traject
    if (edgeDetectorField.rising || traject->empty()) {
        // load the traject
        algorithmMode = static_cast<AlgorithmMode>(getVariable("pc.navigation.mode")->getValue<int>());
        traject->load(Field::checkFieldName(getVariable("pc.field.name")->getValue<string>()), 
                    getPlatform().gps.utm_zone, 
                    getVariable("pc.navigation.spin_angle")->getValue<double>(),
                    getVariable("pc.purepursuit.inter_point_distance")->getValue<double>(),
                    getVariable("pc.navigation.turning_radius")->getValue<double>(),
                    algorithmModeToInterpolationType[algorithmMode]);

        if (traject->empty()) {
            LoggerStream::getInstance() << DEBUG << "Traject loaded failed";
        } else {
            LoggerStream::getInstance() << DEBUG << "Traject loaded successfully";
        }

        // takes too long to write this to redis skip it for now
        // getStream().setRedisJsonValue("traject", traject->toJson());
    }
    if (traject->empty()) {
        // Stop the robot navigation
        getVariable("plc.control.navigation.end_reached")->setValue<bool>(true);
        return;
    }

    // set the interpolation type if change in algorithm mode
    algorithmMode = static_cast<AlgorithmMode>(getVariable("pc.navigation.mode")->getValue<int>());
    changeDetectorAlgorithmMode.detect(algorithmMode);
    if (changeDetectorAlgorithmMode.changed) {
        LoggerStream::getInstance() << INFO << "Navigation mode changed to " << algorithmMode;
        traject->setInterpolation(algorithmModeToInterpolationType[algorithmMode]);
        resetPosition();
        navigationControl.reset();
        if (algorithmMode == AlgorithmMode::EXTERNAL) {
            navigationControl.setVelocityOperation();  // Reset velocity if pressing external!
        }
    }

    // detect overtake of automode from plc or end of traject
    if (edgeDetectorAutomode.falling || autoModeError  || autoModeReset) {
        // Generate pulse for the plc to stop the robot
        autoModeReset = true;
        if (trajectEndReachedPulse.generatePulse(300ms)) {
            LoggerStream::getInstance() << DEBUG <<"Pulse to terminate auto mode";
            getVariable("plc.control.navigation.end_reached")->setValue<bool>(true);
            getVariable("pc.simulation.auto")->setValue<bool>(false);
            navigationControl.setVelocityOperation();  // reset velocities
        } else {
            getVariable("plc.control.navigation.end_reached")->setValue<bool>(false);
            autoModeReset = false;
            autoModeError = false;
        }
    }

    // detect creation or loading of new field.
    if (edgeDetectorAutomode.rising || edgeDetectorField.rising) {
        LoggerStream::getInstance() << INFO <<"Resetting the field: ";
        if (edgeDetectorAutomode.rising) {
            LoggerStream::getInstance() << INFO <<"Auto mode is started.";
        } else if (edgeDetectorField.rising) {
            LoggerStream::getInstance() << INFO <<"New traject is set";
        }
        
        try {
	        navigationControl.reset();
        } catch(const TrajectLengthIsZero& e) {
            getVariable("pc.execution.notification")->setValue(e.what());
            autoModeError = true;
        } 
    }

    // ** UPDATE **
    // update position
    updatePosition();

    // update navigation control
    if (activeAuto && !autoModeReset && !autoModeError) 
    {
        try {
            navigationControl.update(algorithmMode, edgeDetectorAutomode.rising);
        } catch(const TrajectLengthIsZero& e) {
            getVariable("pc.execution.notification")->setValue("Traject is empty!");
            autoModeError = true;
        } catch(const NoRtkFix& e) {
            getVariable("pc.execution.notification")->setValue("No RTK Fix!");
            getVariable("plc.monitor.state.auto")->setValue<bool>(false);
            autoModeError = true;
        } catch(const RobotOutsideGeofence& e) {
            getVariable("pc.execution.notification")->setValue("Robot outside geofence!");
            autoModeError = true;
        } catch(const WrongRobotOrientation& e) {
            getVariable("pc.execution.notification")->setValue("Wrong robot orientation! Rotate to the orientation of the traject!");
            autoModeError = true;
        } catch(const EndOfTrajectIsReached& e)  {
            getVariable("pc.execution.notification")->setValue("End of Traject is reached!");
            autoModeError = true;
        } catch(const exception& e) {
            getVariable("pc.execution.notification")->setValue("An unexpected error occured! " + string(e.what()));
            autoModeError = true;
        }

        if (hitchBusy) {
            navigationControl.setVelocityOperation();
        }
    }

    // TODO also add position data
    getStream().setRedisJsonValue("navigation.controller.info", position->toJson(platform.gps.utm_zone));
    setRedisJsonStatus(platform);  
}

int main() {
    // First check if ILVO_PATH environment variable is set
    if (getenv("ILVO_PATH") == NULL) { 
        throw EnvVariableNotFoundException("$ILVO_PATH");
    }

    string procName = "ilvo-navigation";
    LoggerStream::createInstance(procName);
    LoggerStream::getInstance() << INFO << "Navigation Logger started";
    Navigation navbase(procName);
    navbase.run();
    return 0;  
}
