#pragma once

#include <Navigation/SensorNode.h>
#include <Utils/Redis/VariableManager.h>

namespace Ilvo {
namespace Core {

    class CameraSensorNode : public SensorNode
    {
    private:
        Utils::Redis::VariableManager* manager;

    public:
        CameraSensorNode(Utils::Redis::VariableManager* manager);
        ~CameraSensorNode() = default;

        // Offset reuses PP path errors (camera row detection not yet implemented)
        Eigen::Vector3d getOffset() override;

        // Reads pc.camera.accuracy from Redis — set externally to simulate camera quality
        double accuracy() override;
    };

} // Core
} // Ilvo
