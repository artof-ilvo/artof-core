#include <Navigation/ControllerNode.h>

using namespace Ilvo::Core;
using namespace Eigen;

Ilvo::Core::ControllerNode::ControllerNode()
{

}


double Ilvo::Core::ControllerNode::accuracy()
{

}

void Ilvo::Core::ControllerNode::runAlgorithm(Vector3d& commands, double ofssetOrientation, double offsetLateral)
{

    commands.x = ...
    commands.y = ...
    commands.z = ...
}

Eigen::Vector3d Ilvo::Core::ControllerNode::update(SensorNode& sensor)
{
    // 1. Get parameters from redis

    // 2. Get offset from sensor

    // 3. run algorithm
    Eigen::Vector3d commands;
    
    runAlgorithm(commands)


}
