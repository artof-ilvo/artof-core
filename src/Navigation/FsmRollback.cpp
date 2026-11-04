#include <Navigation/FsmRollback.h>
#include <Utils/Settings/Platform.h>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;

using namespace std;

void FsmRollback::update()
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
        bool turnNextCorner = position->corners.nextCorner.point.distance(position->currentPoint) <= (manager->getVariable("pc.navigation.turning_radius")->getValue<double>() + 1.5);
        double stopTurnAngle = manager->getValueOr<double>("pc.navigation.stop_turn_angle", 0.0);
        double earlyStoppingAngle = turnNextCorner ? 0.0 : stopTurnAngle;
        if (!turn(earlyStoppingAngle)) {
            // is there another corner nearby? Creep backwards to come in a good position to take this corner
            if (turnNextCorner) {                
                LoggerStream::getInstance() << DEBUG << "TURN -> CREEP_TO_POSITION";
                algorithm.fsmState = CREEP_TO_POSITION;
            } else {
                LoggerStream::getInstance() << DEBUG << "TURN -> ALIGN";
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
