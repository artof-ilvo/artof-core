#pragma once

#include <Navigation/Node.h>
#include <Navigation/SensorNode.h>
#include <Utils/Redis/VariableManager.h>
#include <Utils/Settings/Platform.h>
#include <ThirdParty/Eigen/Dense>
#include <memory>

namespace Ilvo {
namespace Core {

    class ActivateActionNode : public ActionNode {
    private:
        Utils::Redis::VariableManager* manager;
        std::shared_ptr<SensorNode> sensor;
        Utils::Settings::SensorMode sensorMode;
        Utils::Settings::AlgorithmMode algorithmMode;

    public:
        ActivateActionNode(Utils::Redis::VariableManager* manager,
                           std::shared_ptr<SensorNode> sensor,
                           Utils::Settings::SensorMode sensorMode,
                           Utils::Settings::AlgorithmMode algorithmMode);

        // Stops the robot, activates the sensor and algorithm, returns SUCCESS
        Status tick() override;
    };

} // Core
} // Ilvo
