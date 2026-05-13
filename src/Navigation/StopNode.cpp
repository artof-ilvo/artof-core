#include <Navigation/StopNode.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Redis;

StopNode::StopNode(Utils::Redis::VariableManager* manager)
    : manager(manager)
{
}

Status StopNode::tick()
{
    manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(0.0);
    manager->getVariable("plc.control.navigation.velocity.angular")->setValue<double>(0.0);
    return Status::SUCCESS;
}
