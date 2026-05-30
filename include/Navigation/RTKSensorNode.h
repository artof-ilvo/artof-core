#pragma once

#include <Navigation/SensorNode.h>
#include <Utils/Redis/VariableManager.h>

namespace Ilvo {
namespace Core {
    class RTKSensorNode : public SensorNode
    {
    private:
        Utils::Redis::VariableManager* manager;

    public:
        RTKSensorNode(Utils::Redis::VariableManager* manager);
        ~RTKSensorNode() = default;

        Eigen::Vector3d getOffset() override;
        double accuracy() override;
    };
}
}
