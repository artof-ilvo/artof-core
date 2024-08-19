#pragma once

#include <chrono>

namespace Ilvo {
namespace Utils {
namespace Pid {

    class PidController
    {
    private:
        double kp;
        double ki;
        double kd;

        double previousError;
        double integral;
        double derivative;

        bool saturationEnabled;
        double saturationMax;
        double saturationMin;

        std::chrono::steady_clock::time_point lastTime;
    public:
        PidController();
        ~PidController() = default;

        void setParameters(double kp, double ki, double kd);

        double getIntegral();
        double getDerivative();

        void setSaturation(double min, double max);
        bool isSaturationEnabled();

        double update(double error);
        void reset();
    };
    
} 
}
}
