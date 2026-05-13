#include <Navigation/AccuracyConditionNode.h>

using namespace Ilvo::Core;

AccuracyConditionNode::AccuracyConditionNode(std::shared_ptr<SensorNode> sensor, double threshold)
    : sensor(sensor), threshold(threshold)
{
}

Status AccuracyConditionNode::tick()
{
    if (sensor->accuracy() >= threshold)
        return Status::SUCCESS;
    return Status::FAILURE;
}
