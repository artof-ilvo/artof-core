#include <Navigation/ActivateActionNode.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Settings;

ActivateActionNode::ActivateActionNode(VariableManager* manager,
                                       std::shared_ptr<SensorNode> sensor,
                                       AlgorithmMode algorithmMode)
    : manager(manager), sensor(sensor), algorithmMode(algorithmMode)
{
}

Status ActivateActionNode::tick()
{
    int currentMode = manager->getVariable("pc.navigation.mode")->getValue<int>();

    // Stop robot enkel bij algoritme wissel
    if (currentMode != static_cast<int>(algorithmMode)) {
        manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(0.0);
        manager->getVariable("plc.control.navigation.velocity.angular")->setValue<double>(0.0);
    }

    manager->getVariable("pc.navigation.mode")->setValue<int>(static_cast<int>(algorithmMode));

    return Status::SUCCESS;
}
