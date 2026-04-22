#include <Navigation/SensorNode.h>
#include <Utils/Settings/Traject.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Settings;

SensorNode::SensorNode() {}

double SensorNode::offset(Traject& traject)
{
    return 0.0;
}

double SensorNode::accuracy()
{
    return 0.0;
}