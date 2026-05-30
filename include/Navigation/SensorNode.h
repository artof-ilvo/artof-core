#pragma once

#include <ThirdParty/Eigen/Dense>
#include <Utils/Settings/Traject.h>

namespace Ilvo {
namespace Core {
    class SensorNode
    {
    public:
        virtual ~SensorNode() = default;

        virtual Eigen::Vector3d getOffset() = 0;
        virtual double accuracy();
    };
}
}
