#include <Navigation/Node.h>

using namespace Ilvo::Core;

void CompositeNode::addChild(std::shared_ptr<Node> child)
{
    children.push_back(child);
}
