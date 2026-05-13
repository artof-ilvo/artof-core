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

    // Composite nodes hebben children
    class CompositeNode : public Node {
    protected:
        std::vector<std::shared_ptr<Node>> children;
    public:
        void addChild(std::shared_ptr<Node> child);
    };

    // Leaf nodes hebben geen children
    class ConditionNode : public Node {
    public:
        // Geeft enkel SUCCESS of FAILURE, nooit RUNNING
        virtual Status tick() = 0;
    };

    class ActionNode : public Node {
    public:
        // Kan SUCCESS, FAILURE of RUNNING geven
        virtual Status tick() = 0;
    };

} // Core
} // Ilvo
