#pragma once

#include <Utils/Settings/Traject.h>

namespace Ilvo {
namespace Core {
    class SensorNode
    {
    private:

    public:
        SensorNode(/* args */);
        ~SensorNode();

        double offset(Utils::Settings::Traject& traject);
        double accuracy();
    };
}
}