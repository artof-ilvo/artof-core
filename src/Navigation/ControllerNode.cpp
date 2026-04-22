#include <Navigation/ControllerNode.h>

using namespace Ilvo::Core;
using namespace Eigen;

Ilvo::Core::ControllerNode::ControllerNode()
{

}


double Ilvo::Core::ControllerNode::accuracy()
{
    return 0.0;
}

void Ilvo::Core::ControllerNode::runAlgorithm(Vector3d& commands, double ofssetOrientation, double offsetLateral)
{

    // commands.x = ...
    // commands.y = ...
    // commands.z = ...
}

Eigen::Vector3d Ilvo::Core::ControllerNode::update(SensorNode& sensor)
{
    // 1. Get parameters from redis

    // 2. Get offset from sensor
    
    // 3. calcate the offset (lateral and orientation) to the trajectory

    // 3. run algorithm

    // Eigen::Vector3d commands;
    // runAlgorithm(commands)

    return {0.0, 0.0, 0.0};
}
