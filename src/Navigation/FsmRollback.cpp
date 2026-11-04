#include <Navigation/FsmRollback.h>
#include <Utils/Settings/Platform.h>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;

using namespace std;

void FsmRollback::startTurn()
{
    // update next corner
    nextCorner();
    algorithm.headingGoal = toRobotFrame(traject->absPathOrientation(position->corners.previousCorner.index+3) );

    // Decided once per turn: the distance to the next corner changes while turning
    bool turnNextCorner = position->corners.nextCorner.point.distance(position->currentPoint) <= (manager->getVariable("pc.navigation.turning_radius")->getValue<double>() + 1.5);
    if (turnNextCorner) {
        // turn with the turning radius until the heading matches, then creep back
        algorithm.fsmState = TURN_DEAD_RECKONING;
        setTurningVelocity();
    } else {
        // drive to where an arc with the turning radius joins the next segment
        algorithm.fsmState = TURN_ADAPTIVE;
        setTurnGoal();
    }
}

void FsmRollback::update()
{
    switch (algorithm.fsmState)
    {
    case STRAIGHTLINE:{
        if (!straightLine(0.3)) { // corner detected in path || driving to last point
            // When turning state, disable implement operation when turning
            manager->getVariable("pc.implement.disable")->setValue(true);
            startTurn();
            LoggerStream::getInstance() << DEBUG << "STRAIGHTLINE -> " << fsmStateToString[algorithm.fsmState];

            // set reset path distance temorarily larger
            position->resetPathDistance = RESET_PATH_DISTANCE_LARGE;
        }
        break;}
    case TURN_DEAD_RECKONING:{
        bool turnNextCorner = position->corners.nextCorner.point.distance(position->currentPoint) <= (manager->getVariable("pc.navigation.turning_radius")->getValue<double>() + 1.5);
        double stopTurnAngle = manager->getValueOr<double>("pc.navigation.stop_turn_angle", 0.0);
        if (!turnDeadReckoning(turnNextCorner ? 0.0 : stopTurnAngle)) {
            // is there another corner nearby? Creep backwards to come in a good position to take this corner
            if (turnNextCorner) {
                LoggerStream::getInstance() << DEBUG << "TURN_DEAD_RECKONING -> CREEP_TO_POSITION";
                algorithm.fsmState = CREEP_TO_POSITION;
            } else {
                LoggerStream::getInstance() << DEBUG << "TURN_DEAD_RECKONING -> ALIGN";
                algorithm.fsmState = ALIGN;
            }
        }
        break;}
    case TURN_ADAPTIVE:{
        if (!turnAdaptive()) {
            // is there another corner nearby? Creep backwards to come in a good position to take this corner
            bool turnNextCorner = position->corners.nextCorner.point.distance(position->currentPoint) <= (manager->getVariable("pc.navigation.turning_radius")->getValue<double>() + 1.5);
            if (turnNextCorner) {
                LoggerStream::getInstance() << DEBUG << "TURN_ADAPTIVE -> CREEP_TO_POSITION";
                algorithm.fsmState = CREEP_TO_POSITION;
            } else {
                LoggerStream::getInstance() << DEBUG << "TURN_ADAPTIVE -> ALIGN";
                algorithm.fsmState = ALIGN;
            }
        }
        break;}
    case ALIGN: {
        if (!align()) {
            LoggerStream::getInstance() << DEBUG << "ALIGN -> STRAIGHTLINE";
            toStraightLine();

            // set reset path distance again to the default
            position->resetPathDistance = RESET_PATH_DISTANCE_DEFAULT;
        }
        break;}
    case CREEP_TO_POSITION: {
        // search for new intersection
        if (!creepToPosition()) {
            startTurn();
            LoggerStream::getInstance() << DEBUG << "CREEP_TO_POSITION -> " << fsmStateToString[algorithm.fsmState];
        }
        break;}
    default:
        break;
    }
}
