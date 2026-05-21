#pragma once

#include <Navigation/Node.h>
#include <Utils/Redis/VariableManager.h>

namespace Ilvo {
namespace Core {

    class StopNode : public ActionNode {
    private:
        Utils::Redis::VariableManager* manager;

    public:
        StopNode(Utils::Redis::VariableManager* manager);

        // Stops the robot, always returns SUCCESS
        Status tick() override;
    };

} // Core
} // Ilvo
