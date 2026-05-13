#pragma once

#include <Navigation/Node.h>
#include <Navigation/SensorNode.h>
#include <memory>

namespace Ilvo {
namespace Core {

    class AccuracyConditionNode : public ConditionNode {
    private:
        std::shared_ptr<SensorNode> sensor;
        double threshold;

    public:
        AccuracyConditionNode(std::shared_ptr<SensorNode> sensor, double threshold);

        // SUCCESS als accuracy >= threshold, anders FAILURE
        Status tick() override;
    };

} // Core
} // Ilvo
