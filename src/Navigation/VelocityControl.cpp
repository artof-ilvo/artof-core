#include <Navigation/VelocityControl.h>
#include <math.h>

using namespace Ilvo::Core;


VelocityControl::VelocityControl(double maxLinearAcc, double maxAngularAcc) : maxLinearAcc(maxLinearAcc), maxAngularAcc(maxAngularAcc)
{}

// Helper to clamp a scalar step towards a target
double VelocityControl::stepTowards(double current, double target, double maxStep) {
    double delta = target - current;
    if (std::abs(delta) <= maxStep) {
        return target;
    }
    return current + (delta > 0 ? maxStep : -maxStep);
}


void VelocityControl::update(double &lonVel, double &latVel, double &omega, double dt_secs)
{
    // Linear acceleration
    double targetLon = lonVel;
    double targetLat = latVel;

    double dLon = targetLon - lonVelMem;
    double dLat = targetLat - latVelMem;
    double requiredAcc = std::sqrt(dLon * dLon + dLat * dLat) / dt_secs;

    if (requiredAcc > maxLinearAcc && requiredAcc > 0.0) {
        double maxStep = maxLinearAcc * dt_secs;
        // Scale the change vector to fit within maxLinearAcc
        double scale = maxStep / (requiredAcc * dt_secs);
        lonVel = lonVelMem + dLon * scale;
        latVel = latVelMem + dLat * scale;
    } else {
        lonVel = targetLon;
        latVel = targetLat;
    }

    // Angular acceleration
    double maxAngStep = maxAngularAcc * dt_secs;
    omega = stepTowards(angVelMem, omega, maxAngStep);

    // 3. Update memory state
    lonVelMem = lonVel;
    latVelMem = latVel;
    angVelMem = omega;
}
