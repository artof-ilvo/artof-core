#include <Navigation/BehaviourTreeBuilder.h>
#include <Exceptions/RobotExceptions.hpp>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Settings;
using namespace nlohmann;

BehaviourTreeBuilder::BehaviourTreeBuilder(VariableManager* manager, double accuracyThreshold)
    : manager(manager), accuracyThreshold(accuracyThreshold)
{
}

std::shared_ptr<SensorNode> BehaviourTreeBuilder::buildSensor(const std::string& sensorId)
{
    if (sensorId == "rtk")
        return std::make_shared<RTKSensorNode>(manager);
    // if (sensorId == "camera") return std::make_shared<CameraSensorNode>(manager);
    throw std::runtime_error("Unknown sensor: " + sensorId);
}

AlgorithmMode BehaviourTreeBuilder::buildAlgorithmMode(const std::string& algorithmId)
{
    if (algorithmId == "pure_pursuit")   return AlgorithmMode::PURE_PP;
    if (algorithmId == "pp_spinning_90") return AlgorithmMode::PP_SPINNING_90;
    if (algorithmId == "pp_roll_back")   return AlgorithmMode::PP_ROLL_BACK;
    if (algorithmId == "external")       return AlgorithmMode::EXTERNAL;
    throw std::runtime_error("Unknown algorithm: " + algorithmId);
}

std::shared_ptr<Node> BehaviourTreeBuilder::buildSequence(const std::string& algorithmId, const std::string& sensorId)
{
    auto sensor = buildSensor(sensorId);
    auto algorithmMode = buildAlgorithmMode(algorithmId);

    auto sequence = std::make_shared<SequenceNode>();
    sequence->addChild(std::make_shared<ActivateActionNode>(manager, sensor, algorithmMode));
    sequence->addChild(std::make_shared<AccuracyConditionNode>(sensor, accuracyThreshold));
    return sequence;
}

std::shared_ptr<FallbackNode> BehaviourTreeBuilder::build(const json& segment)
{
    auto tree = std::make_shared<FallbackNode>();

    std::string prefAlgo   = segment["algorithm"].get<std::string>();
    std::string prefSensor = segment["sensor"].get<std::string>();
    tree->addChild(buildSequence(prefAlgo, prefSensor));

    if (segment.contains("fallback")) {
        std::string fbAlgo   = segment["fallback"]["algorithm"].get<std::string>();
        std::string fbSensor = segment["fallback"]["sensor"].get<std::string>();
        tree->addChild(buildSequence(fbAlgo, fbSensor));
    }

    // StopNode as final fallback when all options fail
    tree->addChild(std::make_shared<StopNode>(manager));

    return tree;
}
