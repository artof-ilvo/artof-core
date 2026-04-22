#pragma once

#include <Eigen/Dense>

namespace Ilvo {
namespace Core {
    class SensorNode
    {
    private:

    public:
        SensorNode(/* args */);
        ~SensorNode();

        virtual Eigen::Vector3d getOffset(Traject& traject) = 0;
        double accuracy();
    };
}
}