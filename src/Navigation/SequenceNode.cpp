#include <Navigation/SequenceNode.h>

using namespace Ilvo::Core;

Status SequenceNode::tick()
{
    for (auto& child : children) {
        Status status = child->tick();
        if (status == Status::FAILURE)
            return Status::FAILURE;
        if (status == Status::RUNNING)
            return Status::RUNNING;
    }
    return Status::SUCCESS;
}
