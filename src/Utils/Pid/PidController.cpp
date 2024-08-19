#include <Utils/Pid/PidController.h>

using namespace Ilvo::Utils::Pid;


PidController::PidController()
    : kp(0.0), ki(0.0), kd(0.0), 
    previousError(0.0), integral(0.0), derivative(0.0),
    saturationMax(100.0), saturationMin(-100.0), saturationEnabled(false),
    lastTime(std::chrono::steady_clock::now())
{
}

double PidController::update(double error) {
    auto timestamp = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = timestamp - lastTime;

    // derivative term
    derivative = (error - previousError) / elapsed.count();
    // integral term
    // if the signs of the error and previous error change, reset integral term
    if ((error > 0 && previousError < 0) || (error < 0 && previousError > 0)) {
        // if error changes sign, reset integral
        integral = 0.0;
    } else if (!saturationEnabled) {
        // if saturation is not enabled, update integral
        integral += error * elapsed.count();
    }

    // Calculate the output
    double output = kp * error + ki * integral + kd * derivative;
    if (output > saturationMax) {
        output = saturationMax;
        saturationEnabled = true;
    } else if (output < saturationMin) {
        output = saturationMin;
        saturationEnabled = true;
    } else {
        saturationEnabled = false;
    }

    // Update states
    previousError = error;
    lastTime = timestamp;

    return output;
}

void PidController::setSaturation(double min, double max)
{
    saturationMin = min;
    saturationMax = max;
}


bool PidController::isSaturationEnabled()
{
    return saturationEnabled;
}

void PidController::setParameters(double kp, double ki, double kd)
{
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}


double PidController::getIntegral()
{
    return integral;
}


double PidController::getDerivative()
{
    return derivative;
}


void PidController::reset()
{
    previousError = 0.0;
    integral = 0.0;
    derivative = 0.0;
    lastTime = std::chrono::steady_clock::now();
    saturationEnabled = false;
}