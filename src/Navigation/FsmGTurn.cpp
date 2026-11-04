#include <Navigation/FsmGTurn.h>
#include <Utils/Settings/Platform.h>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;

using namespace std;

void FsmGTurn::update() {
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
                double absoluteLinvel = platform.autoVel.min;
                creepVelocity.set(absoluteLinvel * sin(DegToRad(velocityAngleDegrees)), signY * absoluteLinvel * cos(DegToRad(velocityAngleDegrees)));

                algorithm.fsmState = CREEP_SIDEWAYS;
            } else {
                LoggerStream::getInstance() << DEBUG << "SPINNING -> STRAIGHTLINE";

                toStraightLine();
            }

        }
        break;}
    case CREEP_SIDEWAYS: {
        if (!creepToCorner()) {
            LoggerStream::getInstance() << DEBUG << "CREEP_SIDEWAYS -> STRAIGHTLINE";

            stopLinearOperation();
            toStraightLine();
            nextCorner();
        }
        break;}
    default:
        break;
    }
}
