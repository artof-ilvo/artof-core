#include <Navigation/FallbackNode.h>

using namespace Ilvo::Core;

Status FallbackNode::tick()
{
    for (auto& child : children) {
        Status status = child->tick();
        if (status != Status::FAILURE)
            return status;
    }
    return Status::FAILURE;
}
