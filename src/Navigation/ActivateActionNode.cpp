#include <Navigation/ActivateActionNode.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Settings;

ActivateActionNode::ActivateActionNode(VariableManager* manager,
                                       std::shared_ptr<SensorNode> sensor,
                                       SensorMode sensorMode,
                                       AlgorithmMode algorithmMode)
    : manager(manager), sensor(sensor), sensorMode(sensorMode), algorithmMode(algorithmMode)
{
}

Status ActivateActionNode::tick()
{
    int currentMode = manager->getVariable("pc.navigation.mode")->getValue<int>();

    // Only stop the robot when the algorithm actually changes
    if (currentMode != static_cast<int>(algorithmMode)) {
        manager->getVariable("plc.control.navigation.velocity.longitudinal")->setValue<double>(0.0);
        manager->getVariable("plc.control.navigation.velocity.angular")->setValue<double>(0.0);
    }

    manager->getVariable("pc.navigation.mode")->setValue<int>(static_cast<int>(algorithmMode));
    manager->getStream().setRedisValue("pc.navigation.sensor", std::to_string(static_cast<int>(sensorMode)));

    Eigen::Vector3d offset = sensor->getOffset();
    manager->getStream().setRedisValue("pc.sensor.distance_error",    std::to_string(offset[0]));
    manager->getStream().setRedisValue("pc.sensor.orientation_error", std::to_string(offset[1]));

    return Status::SUCCESS;
}
