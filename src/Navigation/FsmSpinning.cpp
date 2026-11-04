#include <Navigation/FsmSpinning.h>
#include <Utils/Settings/Platform.h>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;

using namespace std;

void FsmSpinning::update() {
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
            toStraightLine();
        }
        break;}
    default:
        break;
    }
}
