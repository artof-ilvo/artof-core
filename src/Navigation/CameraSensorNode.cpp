#include <Navigation/CameraSensorNode.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Settings;

CameraSensorNode::CameraSensorNode(VariableManager* manager)
    : manager(manager)
{
}

Eigen::Vector3d CameraSensorNode::getOffset()
{
    double lateralError     = manager->getVariable("pc.path.distance_error")->getValue<double>();
    double orientationError = manager->getVariable("pc.path.orientation_error")->getValue<double>();
    return Eigen::Vector3d(lateralError, orientationError, 0.0);
}

double CameraSensorNode::accuracy()
{
    // Manual override via Redis for testing: redis-cli SET pc.camera.accuracy 0.0
    try {
        return manager->getVariable("pc.camera.accuracy")->getValue<double>();
    } catch (...) {}

    // 0 m deviation → 1.0, 1 m deviation → 0.0
    double deviation = std::abs(manager->getVariable("pc.path.distance_error")->getValue<double>());
    return std::max(0.0, 1.0 - deviation);
}
