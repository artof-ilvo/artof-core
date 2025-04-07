#include <Navigation/NavigationControl.h>
#include <Navigation/Navigation.h>
#include <ThirdParty/bprinter/table_printer.h>
#include <Exceptions/RobotExceptions.hpp>
#include <Utils/Settings/Platform.h>
#include <math.h> 
#include <memory>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;

using bprinter::TablePrinter;
using namespace std;


void NavigationControl::init(Utils::Redis::VariableManager* manager, shared_ptr<Traject> traject, shared_ptr<PositionData> position) {
    LoggerStream::getInstance() << DEBUG << "Initialize NavigationControl.";
    this->manager = manager;
    this->traject = traject;
    this->position = position;
    // set the velocity operation to zero
    setVelocityOperation();
}

void NavigationControl::reset() {
    LoggerStream::getInstance() << INFO << "Reset algorithm data";
    algorithm.angleToGoal = 0;  // reset smallestAngleDegrees
    algorithm.fsmState = STRAIGHTLINE;  // set defaullt state is pure pursuit 
    algorithm.steadyState = false;  // first enable the rough lateral controller  
    steadyStateLateralController.reset();  // reset the lateral controllers
    roughLateralController.reset();  // reset the lateral controllers
    purepursuitController.reset();  // reset the purepursuit controllers
}

bool NavigationControl::getActiveSideways() const {
    return algorithm.fsmState == CREEP_SIDEWAYS;
}

void NavigationControl::update(AlgorithmMode algorithmMode, bool firstTime) {
    // check if rtk fix
    if (!manager->getVariable("pc.simulation.active")->getValue<bool>()) {
        if (manager->getVariable("pc.gps.fix")->getValue<int>() != 4) {
            throw NoRtkFix();
        } 
    }

    // check if current position is in the geofence polygon
    if (traject->outsideGeofence(position->currentPoint)) {
        throw RobotOutsideGeofence();
    }

    // Only continue when no external navigation algorithm is active
    if (algorithmMode == AlgorithmMode::EXTERNAL) {
        return;
    }

    // check if path has a certain length
    if (traject->interpolationLength() == 0) { // stop when size of path is zero
        throw TrajectLengthIsZero();
    } 

    // check if still on the path
    // double distanceToClosestPoint = position->currentPoint.distance(position->closestPoint);
    // if (distanceToClosestPoint > 2.0) {
    //     throw TrajectLost();
    // }

    // check robot orientation
    double pathOrientation = manager->getVariable("pc.path.orientation")->getValue<double>();
    double robotOrientation = position->robotRefState.getR().asVector()[2];
    double smallestAngle = calcSmallestAngleAbsolute(pathOrientation, robotOrientation);
    // check robot orientation to closest point
    if (smallestAngle > 120 && firstTime) {
        LoggerStream::getInstance() << DEBUG << "pathOrientation: " << pathOrientation << " robotOrientation: " << robotOrientation << " smallestAngle: " << smallestAngle;
        LoggerStream::getInstance() << INFO << "Robot orientation is wrong. The angle to the path is: " << smallestAngle;
        throw WrongRobotOrientation();
    }

    // stop 1m before the end of path is reached
    int numStopPoints = (int)(1.0 / manager->getVariable("pc.purepursuit.inter_point_distance")->getValue<double>());
    if ((position->closestPoint.index+1+numStopPoints) >= traject->interpolationLength()) {
        throw EndOfTrajectIsReached();
    }

    // update velocities
    if (manager->getVariable("pc.simulation.auto")->getValue<bool>() ||
        manager->getVariable("plc.monitor.state.auto")->getValue<bool>() || 
        manager->getVariable("plc.monitor.state.throttle")->getValue<bool>()) {  // set velocities
        algorithm.velocity.longitudinal = manager->getVariable("pc.navigation.non_operational_velocity")->getValue<double>();
        algorithm.longitudinalTaskVelocity = manager->getVariable("pc.navigation.operational_velocity")->getValue<double>();
    } else if (manager->getVariable("plc.monitor.state.steer")->getValue<bool>()) {
        algorithm.velocity.longitudinal = manager->getVariable("plc.monitor.navigation.velocity.longitudinal")->getValue<double>();  // TODO what will become the unit speed check this out
        algorithm.longitudinalTaskVelocity = algorithm.velocity.longitudinal;
    }

    // ** STATE DIAGRAM **
    switch (algorithmMode)
    {
    case PP_SPINNING_90:
        stateMachineSpinning();
        break;
    case PP_SPINNING_180:
        stateMachineGTurn();
        break;
    case PURE_PP:
        stateMachineNoSpinning();
        break;
    case PP_ROLL_BACK:
        stateMachineRollBack();
        break;
    default:
        throw NoNavigationAlgorithmIdFound(algorithmMode);
        break;
    }
}


void NavigationControl::nextCorner() {
    LoggerStream::getInstance() << DEBUG << "Taking next corner with cornerIndex: " << position->corners.nextCorner.cornerIndex+1 << "/" << traject->cornersLength() << ".";
    position->corners = traject->cornersByIndex(position->corners.nextCorner.cornerIndex+1);
}

void NavigationControl::purePursuit() {
    // update carrot point
    double linearVelocity = algorithm.velocity.longitudinal;
    double carrotDistance = manager->getVariable("pc.purepursuit.carrot_distance")->getValue<double>();
    double minCarrotDistance = 1.5;
    double interpolationDistance = manager->getVariable("pc.purepursuit.inter_point_distance")->getValue<double>();

    if (traject->getInterpolationType() == InterpolationType::CURVY) {
        Point* closestPointPtr = traject->getInterpolation().at(position->closestPoint.index).get();
        Point* carrotPointPtr = traject->getInterpolation().at(position->carrotPoint.index).get();
        CurvyPoint* closestCurvyPointPtr = dynamic_cast<CurvyPoint*>(closestPointPtr);
        CurvyPoint* carrotCurvyPointPtr = dynamic_cast<CurvyPoint*>(carrotPointPtr);
        if (closestCurvyPointPtr != nullptr && carrotCurvyPointPtr != nullptr) {
            if (carrotCurvyPointPtr->radius > 0.0 || closestCurvyPointPtr->radius > 0.0) {
                double radius = std::max(carrotCurvyPointPtr->radius, closestCurvyPointPtr->radius);
                linearVelocity = manager->getPlatform().auto_velocity.min;
                carrotDistance = radius;
                if (carrotDistance < minCarrotDistance) carrotDistance = minCarrotDistance;
            }
        } else {
            LoggerStream::getInstance() << DEBUG << "Pure pursuit - closest and carrot point is not a CurvyPoint.";
        }
    }

    // calculate carrot point
    position->carrotPoint = traject->closestPoint(position->currentPoint, position->closestPoint.index, position->closestPoint.index + (int)(carrotDistance / interpolationDistance) + 10, carrotDistance);

    // calculate alpha
    Line lineToCarrot(position->currentPoint, position->carrotPoint);
    double carrotLineOrientationDegree = toRobotFrame( lineToCarrot.alpha() );
    double alphaDegree = carrotLineOrientationDegree - position->heading;
    algorithm.alpha = DegToRad(alphaDegree);
    // calculate distance to path
    double distanceToPath = manager->getVariable("pc.path.distance_error")->getValue<double>();
    // Steady state latch
    if (!algorithm.steadyState && abs(distanceToPath) < 0.2) {
        algorithm.steadyState = true;
        LoggerStream::getInstance() << DEBUG << "Pure pursuit - Steady state reached.";
    } else if (algorithm.steadyState && abs(distanceToPath) > 1.0) {
        algorithm.steadyState = false;
        LoggerStream::getInstance() << DEBUG << "Pure pursuit - Lost steady state.";
    }

    // linear velocity based on taskmap operation
    if (traject->insideAnyTask(position->currentPoint)) {
        linearVelocity = algorithm.longitudinalTaskVelocity;
    }   
    // slow down mode
    if (manager->getVariable("pc.implement.slow_down")->getValue<bool>()) {
        linearVelocity = manager->getPlatform().auto_velocity.min;
    }

    // PID controller to remove steady state errors
    double kp, ki, kd = 0.0;
    double saturationMin, saturationMax = 0.0;
    double currentVelocity = manager->getVariable("plc.monitor.navigation.velocity.longitudinal")->getValue<double>();
    double purePursuitWeightFactor = manager->existsVariable("pc.purepursuit.weight_factor") ? manager->getVariable("pc.purepursuit.weight_factor")->getValue<double>() : 1.0;
    double pidWeightFactor = 1.0 - purePursuitWeightFactor;
    
    double lateralPidOutput = 0.0;  
    double orientationPidOutput = 0.0;

    double angularVelocityPurePursuit = 0.0;
    double curvatureDefault = 2 * sin(algorithm.alpha) / position->currentPoint.distance(position->carrotPoint);
    double curvature = curvatureDefault;
    double curvaturePidFactor = 1.0;

    double lateralVelocity, longitudinalVelocity, angularVelocity = 0.0;

    bool enablePid = pidWeightFactor > 0.0 && currentVelocity > 0.01;

    if (!enablePid) {
        // reset controllers when vehicle is not moving
        steadyStateLateralController.reset();
        roughLateralController.reset();
        purepursuitController.reset();
    } 

    if (manager->getPlatform().navModesContainsId(AlgorithmMode::PP_SPINNING_180)) {
       if (enablePid) {
            // Calculate the errors
            position->headCurrentPoint = position->robotHeadState.getT();
            position->headClosestPoint = traject->closestPoint(position->headCurrentPoint, position->closestPoint.index, position->closestPoint.index + (int)(6.0 / interpolationDistance));
            Line line = traject->pathLine(position->headClosestPoint.index);
            double pidErrorDistance = traject->isPointLeft(position->headClosestPoint.index, position->headCurrentPoint) * line.distance(position->headCurrentPoint);

            // PID for lateral corrections
            saturationMin = -linearVelocity * pidWeightFactor;
            saturationMax = linearVelocity * pidWeightFactor;
            if (algorithm.steadyState) {
                kp = manager->existsVariable("pc.pid_steady_state.p") ? manager->getVariable("pc.pid_steady_state.p")->getValue<double>() : 0.0; 
                ki = manager->existsVariable("pc.pid_steady_state.i") ? manager->getVariable("pc.pid_steady_state.i")->getValue<double>() : 0.0;
                kd = manager->existsVariable("pc.pid_steady_state.d") ? manager->getVariable("pc.pid_steady_state.d")->getValue<double>() : 0.0;
                steadyStateLateralController.setSaturation(saturationMin, saturationMax);
                steadyStateLateralController.setParameters(kp, ki, kd);
                lateralPidOutput = steadyStateLateralController.update(pidErrorDistance);

                manager->getVariable("pc.pid_steady_state.proportional")->setValue<double>(steadyStateLateralController.getProportional());
                manager->getVariable("pc.pid_steady_state.integral")->setValue<double>(steadyStateLateralController.getIntegral());
                manager->getVariable("pc.pid_steady_state.derivative")->setValue<double>(steadyStateLateralController.getDerivative());
                manager->getVariable("pc.pid_steady_state.value")->setValue<double>(steadyStateLateralController.getOutput());
            } else {
                kp = manager->existsVariable("pc.pid_rough.p") ? manager->getVariable("pc.pid_rough.p")->getValue<double>() : 0.0;
                roughLateralController.setSaturation(saturationMin, saturationMax);
                roughLateralController.setParameters(kp, 0.0, 0.0);
                lateralPidOutput = roughLateralController.update(pidErrorDistance);

                manager->getVariable("pc.pid_rough.proportional")->setValue<double>(roughLateralController.getProportional());
                manager->getVariable("pc.pid_rough.integral")->setValue<double>(roughLateralController.getIntegral());
                manager->getVariable("pc.pid_rough.derivative")->setValue<double>(roughLateralController.getDerivative());
                manager->getVariable("pc.pid_rough.value")->setValue<double>(roughLateralController.getOutput());
            }
        } 

        lateralVelocity = -lateralPidOutput;
        longitudinalVelocity = sgn(linearVelocity) * sqrt(pow(linearVelocity, 2) - pow(lateralVelocity, 2)); // speed vactor may not go over the asked speed
        angularVelocityPurePursuit = longitudinalVelocity * curvature;
        setVelocityOperation(longitudinalVelocity, lateralVelocity, angularVelocityPurePursuit);
    } else {
        // Calculate the errors
        double pidErrorOrientation = manager->existsVariable("pc.path.orientation_error") ? manager->getVariable("pc.path.orientation_error")->getValue<double>() : 0.0;  
        
        // PID for angular corrections
        kp = manager->existsVariable("pc.purepursuit.pid.p") ? manager->getVariable("pc.purepursuit.pid.p")->getValue<double>() : 0.0; 
        ki = manager->existsVariable("pc.purepursuit.pid.i") ? manager->getVariable("pc.purepursuit.pid.i")->getValue<double>() : 0.0;
        kd = manager->existsVariable("pc.purepursuit.pid.d") ? manager->getVariable("pc.purepursuit.pid.d")->getValue<double>() : 0.0;
        saturationMin = manager->existsVariable("pc.purepursuit.pid.saturation.min") ? manager->getVariable("pc.purepursuit.pid.saturation.min")->getValue<double>() : -1.0;
        saturationMax = manager->existsVariable("pc.purepursuit.pid.saturation.max") ? manager->getVariable("pc.purepursuit.pid.saturation.max")->getValue<double>() : 1.0;
        purepursuitController.setSaturation(saturationMin, saturationMax);
        purepursuitController.setParameters(kp, ki, kd);
        orientationPidOutput = purepursuitController.update(pidErrorOrientation);
        manager->getVariable("pc.purepursuit.pid.proportional")->setValue<double>(purepursuitController.getProportional());
        manager->getVariable("pc.purepursuit.pid.integral")->setValue<double>(purepursuitController.getIntegral());
        manager->getVariable("pc.purepursuit.pid.derivative")->setValue<double>(purepursuitController.getDerivative());
        manager->getVariable("pc.purepursuit.pid.value")->setValue<double>(purepursuitController.getOutput());
        curvaturePidFactor = 1 + abs(orientationPidOutput);

        curvature = curvatureDefault * curvaturePidFactor;
        longitudinalVelocity = linearVelocity; // forward velocity remains the same
        angularVelocityPurePursuit = longitudinalVelocity * curvature; 
        angularVelocity = angularVelocityPurePursuit; 
        setVelocityOperation(longitudinalVelocity, 0.0, angularVelocity);
    }

    manager->getVariable("pc.purepursuit.curvature_default")->setValue<double>(curvatureDefault);
    manager->getVariable("pc.purepursuit.curvature")->setValue<double>(curvature);
    manager->getVariable("pc.purepursuit.curvature_factor")->setValue<double>(curvaturePidFactor);

}

bool NavigationControl::straightLine(double deaccelerationDistance) {
    int pointsToIntersection = int((manager->getVariable("pc.navigation.turning_radius")->getValue<double>() + deaccelerationDistance) / manager->getVariable("pc.purepursuit.inter_point_distance")->getValue<double>());
    
    purePursuit();

    return (position->closestPoint.index + pointsToIntersection) < position->corners.nextCorner.index;
}

bool NavigationControl::creepToCorner() {
    if (position->closestPoint.index >= position->corners.nextCorner.index) {
        LoggerStream::getInstance() << DEBUG << "Terminate creep to corner mode because the new corners are updated";
	    return false;
    } else {
        double distanceToIntersection = position->currentPoint.distance(position->corners.nextCorner.point);
        double longitudinalVelocity, lateralVelocity;

        if (algorithm.fsmState == CREEP_SIDEWAYS) {
            if (distanceToIntersection < 1.0) {
                lateralVelocity = creepVelocity.lateral * distanceToIntersection;
                if (abs(lateralVelocity) < manager->getPlatform().auto_velocity.min) lateralVelocity = sgn(lateralVelocity) * manager->getPlatform().auto_velocity.min; // saturation on 0.2 m/s
            } else {
                lateralVelocity = creepVelocity.lateral;
            }

            // keep the same ratio for longitudinalVelocity
            longitudinalVelocity = creepVelocity.longitudinal * (lateralVelocity/creepVelocity.lateral);
        } else {
            // drive forward until the turning point
            longitudinalVelocity = creepVelocity.longitudinal * (distanceToIntersection / manager->getVariable("pc.purepursuit.carrot_distance")->getValue<double>());   // speed is proportional with the distance to the intersec point
            if (longitudinalVelocity < manager->getPlatform().auto_velocity.min) longitudinalVelocity = manager->getPlatform().auto_velocity.min; // saturation on manager->getPlatform().auto_velocity.min m/s
            lateralVelocity = 0.0;
        }

        setVelocityOperation(longitudinalVelocity, -lateralVelocity, creepVelocity.angular);
        return true;
    }
}

bool NavigationControl::creepToPosition()
{
    // if last point, creep to last point
    if (position->corners.nextCorner.cornerIndex >= traject->cornersLength()-1) {
        setVelocityOperation(manager->getPlatform().auto_velocity.min);
        return true;
    }

    double distanceToNextCorner = position->corners.nextCorner.point.distance(position->currentPoint);
    double distanceToPreviousCorner = position->corners.previousCorner.point.distance(position->currentPoint);
    bool condition1 = distanceToNextCorner >= manager->getVariable("pc.navigation.turning_radius")->getValue<double>();
    bool condition2 = distanceToPreviousCorner < distanceToNextCorner;

    if (condition1 && condition2) {
        LoggerStream::getInstance() << DEBUG << "distanceToNextCorner >= ppData.rotationRadius, distanceToNextCorner (m): " << distanceToNextCorner << ", rotationRadius (m): " << manager->getVariable("pc.navigation.turning_radius")->getValue<double>();
        LoggerStream::getInstance() << DEBUG << "distanceToPreviousCorner < distanceToNextCorner, distanceToPreviousCorner (m): " << distanceToPreviousCorner << ", distanceToNextCorner (m): " << distanceToNextCorner;
        setVelocityOperation();
        return false;
    } else {
        setVelocityOperation(-manager->getPlatform().auto_velocity.min);
        return true;
    }
}

bool NavigationControl::turn()
{
    double smallestAngle = calcSmallestAngle(position->heading, algorithm.headingGoal);

    if (stopTurning(smallestAngle)) {
        LoggerStream::getInstance() << DEBUG << "yaw: " << position->heading << "°, algorithm.headingGoal: " << algorithm.headingGoal << "°";

        return false;
    } else {
        setVelocityOperation(creepVelocity.longitudinal, -creepVelocity.lateral, creepVelocity.angular);
        return true;
    }
}

bool NavigationControl::spinning() {
    stopLinearOperation();

    double smallestAngle;  // in degrees
    smallestAngle = calcSmallestAngle(position->heading, algorithm.headingGoal);

    if (stopTurning(smallestAngle, 3.0)) {
        LoggerStream::getInstance() << DEBUG << "yaw: " << position->heading << "°, algorithm.headingGoal: " << algorithm.headingGoal << "°";

	    return false;
    } else {
        // slow down when approaching proper corner
	    double spinVel = manager->getVariable("pc.navigation.spinning_velocity")->getValue<double>();
        if (abs(smallestAngle) < 10 ) spinVel *= (1.0/3.0);
        else if (abs(smallestAngle) < 20 ) spinVel *= (1.0/2.0);
        else if (abs(smallestAngle) < 30 ) spinVel *= (2.0/3.0);

        manager->getVariable("plc.control.navigation.velocity.angular")->setValue<double>(- sgn(smallestAngle) * spinVel);

        return true;
    }
}

bool NavigationControl::stopTurning(double smallestAngle, double earlyStoppingAngle)
{
    bool condition1 = abs(smallestAngle) < earlyStoppingAngle; // early stopping (based on dynamics of cimat)
    bool condition2 = false;
    
    if (algorithm.angleToGoal != 0 && abs(smallestAngle) < 15) {
        condition2 = abs(algorithm.angleToGoal) < abs(smallestAngle);
    }

    bool condition = condition1 || condition2;
    if (condition) { 
        if (condition1) LoggerStream::getInstance() << DEBUG << "* yaw - robotAngleDegrees < 5 * algorithm.headingGoal: " << algorithm.headingGoal;
        if (condition2) LoggerStream::getInstance() << DEBUG << "* smallestAngle >= smallestAngleDegrees * smallestAngle: " << abs(smallestAngle) << " - smallestAngleDegrees: " << abs(algorithm.angleToGoal);

        setVelocityOperation();
        algorithm.angleToGoal = 0.0;
    } else {
        algorithm.angleToGoal = smallestAngle;
    }

    return condition;
}

void NavigationControl::stateMachineSpinning() {
    switch (algorithm.fsmState)
    {
    case STRAIGHTLINE: {
        if (!straightLine()) { 
            LoggerStream::getInstance() << DEBUG << "STRAIGHTLINE -> CREEP_TO_CORNER";
            creepVelocity.set(algorithm.velocity.longitudinal);
            algorithm.fsmState = CREEP_TO_CORNER;
        }
        break;}
    case CREEP_TO_CORNER: {
        if (!creepToCorner()) {
            LoggerStream::getInstance() << DEBUG << "CREEP_TO_CORNER -> SPINNING";
            algorithm.turn180 = false;
            algorithm.fsmState = SPINNING;
            // When spinning state, disable implement operation when spinning
            manager->getVariable("pc.implement.disable")->setValue(true);
            // Update corner and set heading goal
            nextCorner();
            algorithm.headingGoal = toRobotFrame(traject->absPathOrientation(position->corners.previousCorner.index+3) );
        }
        break;}
    case SPINNING: {
        if (!spinning()) {
            LoggerStream::getInstance() << DEBUG << "SPINNING -> STRAIGHTLINE";
            algorithm.fsmState = STRAIGHTLINE;

            // reset the lateral controllers
            algorithm.steadyState = false;
            steadyStateLateralController.reset();
            roughLateralController.reset();
            // When going straightline state, enable implement operation again
            manager->getVariable("pc.implement.disable")->setValue(false);
        }
        break;}
    default:
        break;
    }
}

void NavigationControl::stateMachineNoSpinning() {
    // update corners based on the closest point index
    if (position->closestPoint.index >= position->corners.nextCorner.index) {
        nextCorner();
    }
    // always pure pursuit
    purePursuit();
}

void NavigationControl::stateMachineGTurn() {
    switch (algorithm.fsmState)
    {
    case STRAIGHTLINE:{
        if (!straightLine()) {
            LoggerStream::getInstance() << DEBUG << "STRAIGHTLINE -> CREEP_TO_CORNER";

            creepVelocity.set(algorithm.velocity.longitudinal);
            algorithm.fsmState = CREEP_TO_CORNER;
            break;
        }
        break;}
    case CREEP_TO_CORNER:{
        if (!creepToCorner()) {
            LoggerStream::getInstance() << DEBUG << "CREEP_TO_CORNER -> SPINNING";

            // Update corner and set heading goal
            nextCorner();
            if (position->corners.previousCorner.isHeadland) {
                algorithm.headingGoal = toRobotFrame(traject->absPathOrientation(position->corners.nextCorner.index+3) );
                algorithm.turn180 = true;
                LoggerStream::getInstance() << DEBUG << "HEADACRE DETECTED: TURN 180 DEGREES";
            } else {
                algorithm.turn180 = false;
                algorithm.headingGoal = toRobotFrame(traject->absPathOrientation(position->corners.previousCorner.index+3) );
                LoggerStream::getInstance() << DEBUG << "NO HEADACRE DETECTED: TURN 90 DEGREES";
            }
            algorithm.fsmState = SPINNING;
            // When going to spinning state, disable implement operation when spinning
            manager->getVariable("pc.implement.disable")->setValue(true);
        }
        break;}
    case SPINNING:{
        if (!spinning()) {
            if (algorithm.turn180) {
                LoggerStream::getInstance() << DEBUG << "SPINNING -> CREEP_SIDEWAYS";
                // set creep operation data
                double newPathOrientation = traject->absPathOrientation(position->corners.nextCorner.index+3);
                double angleDifferenceDegree = abs(calcSmallestAngle(algorithm.headingGoal, newPathOrientation));
                double velocityAngleDegrees = 90 - angleDifferenceDegree;
                double signY = traject->isPointLeft(position->corners.nextCorner.index+1, position->currentPoint);
                double absoluteLinvel = manager->getPlatform().auto_velocity.min;
                creepVelocity.set(absoluteLinvel * sin(DegToRad(velocityAngleDegrees)), signY * absoluteLinvel * cos(DegToRad(velocityAngleDegrees)));

                algorithm.fsmState = CREEP_SIDEWAYS;
            } else {
                LoggerStream::getInstance() << DEBUG << "SPINNING -> STRAIGHTLINE";

                algorithm.fsmState = STRAIGHTLINE;

                // reset the lateral controllers
                algorithm.steadyState = false;
                steadyStateLateralController.reset();
                roughLateralController.reset();
                // When going straightline state, enable implement operation again
                manager->getVariable("pc.implement.disable")->setValue(false);
            }

        }
        break;}
    case CREEP_SIDEWAYS: {
        if (!creepToCorner()) {
            LoggerStream::getInstance() << DEBUG << "CREEP_SIDEWAYS -> STRAIGHTLINE";

            stopLinearOperation();
            algorithm.fsmState = STRAIGHTLINE;

            // reset the lateral controllers
            algorithm.steadyState = false;
            steadyStateLateralController.reset();
            roughLateralController.reset();
            // When going straightline state, enable implement operation again
            manager->getVariable("pc.implement.disable")->setValue(false);
            nextCorner();
        }
        break;}
    default:
        break;
    }
}

void NavigationControl::setTurningVelocity()
{
    double longitudinalVelcoity = manager->getPlatform().auto_velocity.min; // safety speed
    double signOmega = -traject->isPointLeft(position->corners.previousCorner.index-3, position->corners.nextCorner.point);
    double angularVelocity = signOmega * manager->getPlatform().auto_velocity.min / manager->getVariable("pc.navigation.turning_radius")->getValue<double>();
    creepVelocity.set(longitudinalVelcoity, 0.0, angularVelocity);
}

void NavigationControl::stateMachineRollBack()
{
    switch (algorithm.fsmState)
    {
    case STRAIGHTLINE:{
        if (!straightLine(0.3)) { // corner detected in path || driving to last point
            LoggerStream::getInstance() << DEBUG << "STRAIGHTLINE -> TURN";

            algorithm.fsmState = TURN;
            // When turning state, disable implement operation when turning
            manager->getVariable("pc.implement.disable")->setValue(true);

            // update next corner
            nextCorner();
            algorithm.headingGoal = toRobotFrame(traject->absPathOrientation(position->corners.previousCorner.index+3) );
            // set turning velocity
            setTurningVelocity();

            // set reset path distance temorarily larger
            position->resetPathDistance = RESET_PATH_DISTANCE_LARGE;
        }
        break;}
    case TURN:{
        if (!turn()) {
            // is there another corner nearby? Creep backwards to come in a good position to take this corner
            if (position->corners.nextCorner.point.distance(position->currentPoint) <= (manager->getVariable("pc.navigation.turning_radius")->getValue<double>())) {                
                LoggerStream::getInstance() << DEBUG << "TURN -> CREEP_TO_POSITION";
                algorithm.fsmState = CREEP_TO_POSITION;
            } else {
                LoggerStream::getInstance() << DEBUG << "TURN -> STRAIGHTLINE";
                algorithm.fsmState = STRAIGHTLINE;

                // When straightline state, enable implement operation when straightline
                manager->getVariable("pc.implement.disable")->setValue(false);

                // reset the lateral controllers
                algorithm.steadyState = false;
                steadyStateLateralController.reset();
                roughLateralController.reset();

                // set reset path distance again to the default
                position->resetPathDistance = RESET_PATH_DISTANCE_DEFAULT;
            }
        }
        break;}
    case CREEP_TO_POSITION: {
        // search for new intersection
        if (!creepToPosition()) {
            LoggerStream::getInstance() << DEBUG << "CREEP_TO_POSITION -> TURN";
            algorithm.fsmState = TURN;

            // update next corner
            nextCorner();
            algorithm.headingGoal = toRobotFrame(traject->absPathOrientation(position->corners.previousCorner.index+3) );
            // set turning velocity
            setTurningVelocity();
        }
        break;}
    default:
        break;
    }
}

void NavigationControl::stopLinearOperation()
{
    manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(0);
    manager->getVariable("plc.control.navigation.velocity.lateral")->setValue<double>(0);
}

void NavigationControl::setVelocityOperation(double longitudinalVelocity, double lateralVelocity, double omega)
{
    manager->getVariable("plc.control.navigation.velocity.lateral")->setValue<double>(0);
            
    manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(longitudinalVelocity);
    manager->getVariable("plc.control.navigation.velocity.angular")->setValue<double>(omega);

    if (manager->getPlatform().navModesContainsId(AlgorithmMode::PP_SPINNING_180)) {
        manager->getVariable("plc.control.navigation.sideways")->setValue<bool>(getActiveSideways());
        manager->getVariable("plc.control.navigation.velocity.lateral")->setValue<double>(lateralVelocity);
    } else {
        manager->getVariable("plc.control.navigation.sideways")->setValue<bool>(false);
        manager->getVariable("plc.control.navigation.velocity.lateral")->setValue<double>(0.0);
    }
}

const AlgorithmData& NavigationControl::getAlgorithmData()
{
    return algorithm;
}