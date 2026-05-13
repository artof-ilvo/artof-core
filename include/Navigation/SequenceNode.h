#pragma once

#include <Navigation/Node.h>

namespace Ilvo {
namespace Core {

    class SequenceNode : public CompositeNode {
    public:
        Status tick() override;
    };

} // Core
} // Ilvo
