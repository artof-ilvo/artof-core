#include <Navigation/VelocityControl.h>
#include <Utils/Logging/LoggerStream.h>
#include <math.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Logging;


VelocityControl::VelocityControl(double maxLinearAcc, double maxAngularAcc) : maxLinearAcc(maxLinearAcc), maxAngularAcc(maxAngularAcc)
{}


void VelocityControl::update(double targetLon, double targetLat, double targetOmega, 
                            double &outLon, double &outLat, double &outOmega, 
                            double dt_secs)
{
    // TODO check that it works
    

    // 1. Linear acceleration limiting
    double dLon = targetLon - lonVelMem;
    double dLat = targetLat - latVelMem;
    double requiredAcc = std::sqrt(dLon * dLon + dLat * dLat) / dt_secs;



    if (requiredAcc > maxLinearAcc && requiredAcc > 0.0) {
        double maxStep = maxLinearAcc * dt_secs;
        double scale = maxStep / (requiredAcc * dt_secs);
        outLon = lonVelMem + dLon * scale;
        outLat = latVelMem + dLat * scale;
    } else {
        outLon = targetLon;
        outLat = targetLat;
    }

    // 2. Angular acceleration limiting
    double maxAngStep = maxAngularAcc * dt_secs;

    double delta = targetOmega - angVelMem;
    if (std::abs(delta) <= maxAngStep) {
        outOmega = targetOmega;
    }
    outOmega = angVelMem + (delta > 0 ? maxAngStep : -maxAngStep);

        // Logging all linear acceleration variables
    LoggerStream::getInstance() << DEBUG 
                                << "requiredAcc: " << requiredAcc 
                                << " | maxLinearAcc: " << maxLinearAcc 
                                << " | maxAngularAcc: " << maxAngularAcc 
                                << " | targetLon: " << targetLon 
                                << " | targetLat: " << targetLat 
                                << " | targetOmega: " << targetOmega 
                                << " | outLon" << outLon
                                << " | outLat" << outLat
                                << " | outOmega" << outOmega
                                << " | dt: " << dt_secs;

    // 3. Update memory state
    lonVelMem = outLon;
    latVelMem = outLat;
    angVelMem = outOmega;
}
