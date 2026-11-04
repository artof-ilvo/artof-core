#include <Navigation/Fsm.h>
#include <Exceptions/RobotExceptions.hpp>
#include <Utils/Settings/Platform.h>
#include <math.h> 
#include <algorithm>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;

using namespace std;

Fsm::Fsm(VariableManager* manager, shared_ptr<Traject> traject, shared_ptr<PositionData> position) :
    platform(Platform::getInstance()),
    manager(manager),
    traject(traject),
    position(position)
{}

void Fsm::reset() {
    LoggerStream::getInstance() << INFO << "Reset algorithm data";
    algorithm.angleToGoal = 0;  // reset smallestAngleDegrees
    algorithm.fsmState = STRAIGHTLINE;  // set defaullt state is pure pursuit 
    algorithm.steadyState = false;  // first enable the rough lateral controller  
    steadyStateLateralController.reset();  // reset the lateral controllers
    roughLateralController.reset();  // reset the lateral controllers
    purepursuitController.reset();  // reset the purepursuit controllers
}

bool Fsm::getActiveSideways() const {
    return algorithm.fsmState == CREEP_SIDEWAYS;
}

void Fsm::toStraightLine() {
    algorithm.fsmState = STRAIGHTLINE;

    // reset the lateral controllers
    algorithm.steadyState = false;
    steadyStateLateralController.reset();
    roughLateralController.reset();
    // When going straightline state, enable implement operation again
    manager->getVariable("pc.implement.disable")->setValue(false);
}

void Fsm::nextCorner() {
    LoggerStream::getInstance() << DEBUG << "Taking next corner with cornerIndex: " << position->corners.nextCorner.cornerIndex+1 << "/" << traject->cornersLength() << ".";
    position->corners = traject->cornersByIndex(position->corners.nextCorner.cornerIndex+1);
}

void Fsm::purePursuit(bool slow) {
    // update carrot point
    double linearVelocity = algorithm.velocity.longitudinal;
    double carrotDistance = manager->getVariable("pc.purepursuit.carrot_distance")->getValue<double>();
    double interpolationDistance = manager->getVariable("pc.purepursuit.inter_point_distance")->getValue<double>();

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
    if (slow || manager->getVariable("pc.implement.slow_down")->getValue<bool>()) {
        linearVelocity = platform.autoVel.min;
    }

    // PID controller to remove steady state errors
    double kp = 0.0, ki = 0.0, kd = 0.0;
    double saturationMin = 0.0, saturationMax = 0.0;
    double currentVelocity = manager->getVariable("plc.monitor.navigation.velocity.longitudinal")->getValue<double>();
    double purePursuitWeightFactor = manager->getValueOr<double>("pc.purepursuit.weight_factor", 1.0);
    double pidWeightFactor = 1.0 - purePursuitWeightFactor;
    
    double lateralPidOutput = 0.0;  
    double purePursuitPidOutput = 0.0;

    double angularVelocityPurePursuit = 0.0;
    double curvatureDefault = 2 * sin(algorithm.alpha) / position->currentPoint.distance(position->carrotPoint);
    double curvature = curvatureDefault;

    double lateralVelocity = 0.0, longitudinalVelocity = 0.0, angularVelocity = 0.0;

    bool enableLateralController = manager->getValueOr<bool>("pc.lateral_controller.enable", false);
    bool resetPid = currentVelocity <= 0.01;

    if (resetPid) {
        // reset controllers when vehicle is not moving
        steadyStateLateralController.reset();
        roughLateralController.reset();
        purepursuitController.reset();
    } 

    // Calculate the errors
    position->headCurrentPoint = position->robotHeadState.getT();
    position->headClosestPoint = traject->closestPoint(position->headCurrentPoint, position->closestPoint.index, position->closestPoint.index + (int)(6.0 / interpolationDistance));
    Line line = traject->pathLine(position->headClosestPoint.index);
    double pidErrorDistance = traject->isPointLeft(position->headClosestPoint.index, position->headCurrentPoint) * line.distance(position->headCurrentPoint);

    double pidErrorOrientation = manager->getValueOr<double>("pc.path.orientation_error", 0.0);  

    if (enableLateralController) {
        if (!resetPid && pidWeightFactor > 0.0) {

            // PID for lateral corrections
            saturationMin = -linearVelocity * pidWeightFactor;
            saturationMax = linearVelocity * pidWeightFactor;
            if (algorithm.steadyState) {
                kp = manager->getValueOr<double>("pc.lateral_controller.steady_state.p", 0.0); 
                ki = manager->getValueOr<double>("pc.lateral_controller.steady_state.i", 0.0);
                kd = manager->getValueOr<double>("pc.lateral_controller.steady_state.d", 0.0);
                steadyStateLateralController.setSaturation(saturationMin, saturationMax);
                steadyStateLateralController.setParameters(kp, ki, kd);
                lateralPidOutput = steadyStateLateralController.update(pidErrorDistance);

                manager->getVariable("pc.lateral_controller.steady_state.proportional")->setValue<double>(steadyStateLateralController.getProportional());
                manager->getVariable("pc.lateral_controller.steady_state.integral")->setValue<double>(steadyStateLateralController.getIntegral());
                manager->getVariable("pc.lateral_controller.steady_state.derivative")->setValue<double>(steadyStateLateralController.getDerivative());
                manager->getVariable("pc.lateral_controller.steady_state.value")->setValue<double>(steadyStateLateralController.getOutput());
            } else {
                kp = manager->getValueOr<double>("pc.lateral_controller.rough.p", 0.0);
                roughLateralController.setSaturation(saturationMin, saturationMax);
                roughLateralController.setParameters(kp, 0.0, 0.0);
                lateralPidOutput = roughLateralController.update(pidErrorDistance);

                manager->getVariable("pc.lateral_controller.rough.proportional")->setValue<double>(roughLateralController.getProportional());
                manager->getVariable("pc.lateral_controller.rough.integral")->setValue<double>(roughLateralController.getIntegral());
                manager->getVariable("pc.lateral_controller.rough.derivative")->setValue<double>(roughLateralController.getDerivative());
                manager->getVariable("pc.lateral_controller.rough.value")->setValue<double>(roughLateralController.getOutput());
            }
        } 

        lateralVelocity = -lateralPidOutput;
        longitudinalVelocity = sgn(linearVelocity) * sqrt(pow(linearVelocity, 2) - pow(lateralVelocity, 2)); // speed vactor may not go over the asked speed
        angularVelocityPurePursuit = longitudinalVelocity * curvature;
        setVelocityOperation(longitudinalVelocity, lateralVelocity, angularVelocityPurePursuit);
    } else {        
        // PID for angular corrections
        if (!resetPid && pidWeightFactor > 0.0) {
            kp = manager->getValueOr<double>("pc.purepursuit.pid.p", 0.0); 
            ki = manager->getValueOr<double>("pc.purepursuit.pid.i", 0.0);
            kd = manager->getValueOr<double>("pc.purepursuit.pid.d", 0.0);
            saturationMin = manager->getValueOr<double>("pc.purepursuit.pid.saturation.min", -1.0);
            saturationMax = manager->getValueOr<double>("pc.purepursuit.pid.saturation.max", 1.0);
            purepursuitController.setSaturation(saturationMin, saturationMax);
            purepursuitController.setParameters(kp, ki, kd);
            purePursuitPidOutput = purepursuitController.update(pidErrorDistance);
            manager->getVariable("pc.purepursuit.pid.proportional")->setValue<double>(purepursuitController.getProportional());
            manager->getVariable("pc.purepursuit.pid.integral")->setValue<double>(purepursuitController.getIntegral());
            manager->getVariable("pc.purepursuit.pid.derivative")->setValue<double>(purepursuitController.getDerivative());
            manager->getVariable("pc.purepursuit.pid.value")->setValue<double>(purepursuitController.getOutput());
        }

        longitudinalVelocity = linearVelocity; 
        angularVelocityPurePursuit = longitudinalVelocity * curvature; 
        angularVelocity = purePursuitWeightFactor * angularVelocityPurePursuit + pidWeightFactor * purePursuitPidOutput; 
        setVelocityOperation(longitudinalVelocity, 0.0, angularVelocity);
    }

    manager->getVariable("pc.purepursuit.curvature_default")->setValue<double>(curvatureDefault);
    manager->getVariable("pc.purepursuit.curvature")->setValue<double>(curvature);

}

bool Fsm::straightLine(double deaccelerationDistance) {
    int pointsToIntersection = int((manager->getVariable("pc.navigation.turning_radius")->getValue<double>() + deaccelerationDistance) / manager->getVariable("pc.purepursuit.inter_point_distance")->getValue<double>());
    
    purePursuit();

    return (position->closestPoint.index + pointsToIntersection) < position->corners.nextCorner.index;
}

bool Fsm::creepToCorner() {
    if (position->closestPoint.index >= position->corners.nextCorner.index) {
        LoggerStream::getInstance() << DEBUG << "Terminate creep to corner mode because the new corners are updated";
	    return false;
    } else {
        double distanceToIntersection = position->currentPoint.distance(position->corners.nextCorner.point);
        double longitudinalVelocity, lateralVelocity;

        if (algorithm.fsmState == CREEP_SIDEWAYS) {
            if (distanceToIntersection < 1.0) {
                lateralVelocity = creepVelocity.lateral * distanceToIntersection;
                if (abs(lateralVelocity) < platform.autoVel.min) lateralVelocity = sgn(lateralVelocity) * platform.autoVel.min; // saturation on 0.2 m/s
            } else {
                lateralVelocity = creepVelocity.lateral;
            }

            // keep the same ratio for longitudinalVelocity
            longitudinalVelocity = creepVelocity.longitudinal * (lateralVelocity/creepVelocity.lateral);
        } else {
            // drive forward until the turning point
            longitudinalVelocity = creepVelocity.longitudinal * (distanceToIntersection / manager->getVariable("pc.purepursuit.carrot_distance")->getValue<double>());   // speed is proportional with the distance to the intersec point
            if (longitudinalVelocity < platform.autoVel.min) longitudinalVelocity = platform.autoVel.min; // saturation on platform.autoVel.min m/s
            lateralVelocity = 0.0;
        }

        setVelocityOperation(longitudinalVelocity, -lateralVelocity, creepVelocity.angular);
        return true;
    }
}

bool Fsm::creepToPosition()
{
    // if last point, creep to last point
    if (position->corners.nextCorner.cornerIndex >= traject->cornersLength()-1) {
        setVelocityOperation(platform.autoVel.min);
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
        setVelocityOperation(-platform.autoVel.min);
        return true;
    }
}

bool Fsm::turnDeadReckoning(double earlyStoppingAngle)
{
    double smallestAngle = calcSmallestAngle(position->heading, algorithm.headingGoal);

    if (stopTurning(smallestAngle, earlyStoppingAngle)) {
        LoggerStream::getInstance() << DEBUG << "yaw: " << position->heading << "°, algorithm.headingGoal: " << algorithm.headingGoal << "°";

        return false;
    } else {
        setVelocityOperation(creepVelocity.longitudinal, -creepVelocity.lateral, creepVelocity.angular);
        return true;
    }
}

bool Fsm::turnAdaptive()
{
    double distanceToGoal = position->currentPoint.distance(algorithm.turnGoal);
    if (distanceToGoal < TURN_GOAL_TOLERANCE || position->closestPoint.index >= algorithm.turnGoalIndex) {
        LoggerStream::getInstance() << DEBUG << "Turn goal reached at path index " << position->closestPoint.index << " (goal index " << algorithm.turnGoalIndex << ", " << distanceToGoal << " m from the goal)";
        return false;
    }

    // pure pursuit to the turn goal, no sharper than the turning radius
    Line lineToGoal(position->currentPoint, algorithm.turnGoal);
    double alpha = DegToRad(toRobotFrame(lineToGoal.alpha()) - position->heading);
    double maxCurvature = 1.0 / effectiveTurningRadius();
    double curvature = std::clamp(2 * sin(alpha) / distanceToGoal, -maxCurvature, maxCurvature);
    double longitudinalVelocity = platform.autoVel.min; // safety speed
    setVelocityOperation(longitudinalVelocity, 0.0, longitudinalVelocity * curvature);
    return true;
}

bool Fsm::align()
{
    double angleToPath = calcSmallestAngleAbsolute(position->heading, algorithm.headingGoal);
    double distanceToPath = manager->getVariable("pc.path.distance_error")->getValue<double>();
    if (angleToPath < TURN_ALIGN_ANGLE && abs(distanceToPath) < TURN_ALIGN_DISTANCE) {
        LoggerStream::getInstance() << DEBUG << "Aligned, " << angleToPath << "° and " << distanceToPath << " m to the path at path index " << position->closestPoint.index;
        return false;
    }
    purePursuit(true);
    return true;
}

bool Fsm::spinning() {
    stopLinearOperation();

    double smallestAngle;  // in degrees
    smallestAngle = calcSmallestAngle(position->heading, algorithm.headingGoal);

    double stopTurnAngle = manager->getValueOr<double>("pc.navigation.stop_turn_angle", 0.0);
    if (stopTurning(smallestAngle, stopTurnAngle)) {
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

bool Fsm::stopTurning(double smallestAngle, double earlyStoppingAngle)
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

double Fsm::effectiveTurningRadius()
{
    double turningRadius = manager->getVariable("pc.navigation.turning_radius")->getValue<double>();
    double turningRadiusFactor = manager->getValueOr<double>("pc.navigation.turning_radius_factor", 1.0);
    turningRadiusFactor = turningRadiusFactor > 0.75 ? turningRadiusFactor : 1.0;
    return turningRadius * turningRadiusFactor;
}

void Fsm::setTurningVelocity()
{
    double longitudinalVelocity = platform.autoVel.min; // safety speed
    double signOmega = -traject->isPointLeft(position->corners.previousCorner.index-3, position->corners.nextCorner.point);
    double angularVelocity = signOmega * longitudinalVelocity / effectiveTurningRadius();
    creepVelocity.set(longitudinalVelocity, 0.0, angularVelocity);
}

void Fsm::setTurnGoal()
{
    // nextCorner() was called when the turn started, so the corner being turned is the previous corner
    const CornerPoint& corner = position->corners.previousCorner;
    double turningRadius = effectiveTurningRadius();
    double interpolationDistance = manager->getVariable("pc.purepursuit.inter_point_distance")->getValue<double>();

    // An arc with the turning radius joins the next segment R * tan(deflection / 2) after the corner
    double tangentDistance = 0.0;
    if (!corner.previousRawPoint.isEmpty() && !corner.nextRawPoint.isEmpty()) {
        double inX = corner.point.x() - corner.previousRawPoint.x(), inY = corner.point.y() - corner.previousRawPoint.y();
        double outX = corner.nextRawPoint.x() - corner.point.x(), outY = corner.nextRawPoint.y() - corner.point.y();
        double lengths = hypot(inX, inY) * hypot(outX, outY);
        if (lengths > 0.0) {
            double deflection = acos(std::clamp((inX * outX + inY * outY) / lengths, -1.0, 1.0));
            tangentDistance = turningRadius * tan(deflection / 2);
        }
    }
    // A short next segment (a headland) ends at the next corner
    int goalIndex = corner.index + (int)(tangentDistance / interpolationDistance);
    goalIndex = min(goalIndex, position->corners.nextCorner.index);
    goalIndex = min(goalIndex, traject->interpolationLength() - 1);

    algorithm.turnGoalIndex = goalIndex;
    algorithm.turnGoal = *traject->getInterpolation().at(goalIndex);
    LoggerStream::getInstance() << DEBUG << "Turn goal at path index " << goalIndex << " (" << tangentDistance << " m past corner " << corner.cornerIndex << ")";
}

void Fsm::stopLinearOperation()
{
    manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(0);
    manager->getVariable("plc.control.navigation.velocity.lateral")->setValue<double>(0);
}

void Fsm::setVelocityOperation(double longitudinalVelocity, double lateralVelocity, double omega)
{
    if (platform.robot.config == "4wd4ws") {
        manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(longitudinalVelocity);
        manager->getVariable("plc.control.navigation.velocity.lateral")->setValue<double>(lateralVelocity);
        manager->getVariable("plc.control.navigation.velocity.angular")->setValue<double>(omega);
        manager->getVariable("plc.control.navigation.sideways")->setValue<bool>(getActiveSideways());
    } else if(platform.robot.config == "omni") {
        manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(longitudinalVelocity);
        manager->getVariable("plc.control.navigation.velocity.lateral")->setValue<double>(lateralVelocity);
        manager->getVariable("plc.control.navigation.velocity.angular")->setValue<double>(omega);
    } else {  // "diff" "ackermann" or others
        manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(longitudinalVelocity);
        manager->getVariable("plc.control.navigation.velocity.lateral")->setValue<double>(0.0);            
        manager->getVariable("plc.control.navigation.velocity.angular")->setValue<double>(omega);
    }

}

AlgorithmData& Fsm::getAlgorithmData()
{
    return algorithm;
}
