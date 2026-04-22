#pragma once

#include <map>
#include <string>
#include <Navigation/SensorNode.h>
#include <ThirdParty/Eigen/Dense>

namespace Ilvo {
namespace Core {
    class ControllerNode
    {
    private:
        std::map<std::string, double> params;
        double navigationError;
    public:
        ControllerNode(/* args */);
        ~ControllerNode() = default;

        double accuracy();
        Eigen::Vector3d update(SensorNode& sensor);

        void runAlgorithm(Eigen::Vector3d& commands, double ofssetOrientation, double offsetLateral);
    };
}
}