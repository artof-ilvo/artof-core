#include <Navigation/RTKSensorNode.h>
#include <cmath>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Settings;

RTKSensorNode::RTKSensorNode(VariableManager* manager)
    : manager(manager)
{
}

Eigen::Vector3d RTKSensorNode::getOffset(Traject& traject)
{
    double lateralError      = manager->getVariable("pc.path.distance_error")->getValue<double>();
    double orientationError  = manager->getVariable("pc.path.orientation_error")->getValue<double>();
    return Eigen::Vector3d(lateralError, orientationError, 0.0);
}

double RTKSensorNode::accuracy()
{
    // Accuracy gebaseerd op afwijking t.o.v. traject
    // Bij 0m afwijking → 1.0, bij 1m afwijking → 0.0
    double deviation = std::abs(manager->getVariable("pc.path.distance_error")->getValue<double>());
    return std::max(0.0, 1.0 - deviation);
}
