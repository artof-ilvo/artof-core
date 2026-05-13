#pragma once

#include <Navigation/Node.h>
#include <Navigation/SensorNode.h>
#include <Utils/Redis/VariableManager.h>
#include <Utils/Settings/Platform.h>
#include <memory>

namespace Ilvo {
namespace Core {

    class ActivateActionNode : public ActionNode {
    private:
        Utils::Redis::VariableManager* manager;
        std::shared_ptr<SensorNode> sensor;
        Utils::Settings::AlgorithmMode algorithmMode;

    public:
        ActivateActionNode(Utils::Redis::VariableManager* manager,
                           std::shared_ptr<SensorNode> sensor,
                           Utils::Settings::AlgorithmMode algorithmMode);

        // Stopt robot, activeert sensor + algoritme, geeft SUCCESS
        Status tick() override;
    };

} // Core
} // Ilvo
