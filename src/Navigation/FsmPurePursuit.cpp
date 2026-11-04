#include <Navigation/FsmPurePursuit.h>
#include <Utils/Settings/Platform.h>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;

using namespace std;

void FsmPurePursuit::update() {
    // update corners based on the closest point index
    if (position->closestPoint.index >= position->corners.nextCorner.index) {
        nextCorner();
    }
    // always pure pursuit
    purePursuit();
}
