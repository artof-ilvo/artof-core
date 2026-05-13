#pragma once

#include <Navigation/Node.h>
#include <Navigation/FallbackNode.h>
#include <Navigation/SequenceNode.h>
#include <Navigation/AccuracyConditionNode.h>
#include <Navigation/ActivateActionNode.h>
#include <Navigation/StopNode.h>
#include <Navigation/SensorNode.h>
#include <Navigation/RTKSensorNode.h>
#include <Utils/Redis/VariableManager.h>
#include <Utils/Settings/Platform.h>
#include <ThirdParty/json.hpp>
#include <memory>
#include <string>

namespace Ilvo {
namespace Core {

    class BehaviourTreeBuilder {
    private:
        Utils::Redis::VariableManager* manager;
        double accuracyThreshold;

        std::shared_ptr<SensorNode> buildSensor(const std::string& sensorId);
        Utils::Settings::AlgorithmMode buildAlgorithmMode(const std::string& algorithmId);
        std::shared_ptr<Node> buildSequence(const std::string& algorithmId, const std::string& sensorId);

    public:
        BehaviourTreeBuilder(Utils::Redis::VariableManager* manager, double accuracyThreshold = 0.5);

        // Bouwt een volledige boom voor een segment uit de JSON
        std::shared_ptr<FallbackNode> build(const nlohmann::json& segment);
    };

} // Core
} // Ilvo
