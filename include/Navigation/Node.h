#pragma once

#include <vector>
#include <memory>

namespace Ilvo {
namespace Core {

    enum class Status {
        SUCCESS,
        FAILURE,
        RUNNING
    };

    class Node {
    public:
        virtual ~Node() = default;
        virtual Status tick() = 0;
    };

    // Composite nodes hold child nodes
    class CompositeNode : public Node {
    protected:
        std::vector<std::shared_ptr<Node>> children;
    public:
        void addChild(std::shared_ptr<Node> child);
    };

    // Leaf nodes have no children
    class ConditionNode : public Node {
    public:
        // Returns SUCCESS or FAILURE, never RUNNING
        virtual Status tick() = 0;
    };

    class ActionNode : public Node {
    public:
        // Can return SUCCESS, FAILURE or RUNNING
        virtual Status tick() = 0;
    };

} // Core
} // Ilvo
