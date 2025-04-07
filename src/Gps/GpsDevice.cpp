#include <Gps/GpsDevice.h>
#include <Utils/Timing/Timing.h>
#include <Utils/Geometry/Angle.h>
#include <Utils/Settings/State.h>
#include <Utils/Nmea/Nmea.h>
#include <Utils/Logging/LoggerStream.h>
#include <Exceptions/FileExceptions.hpp>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Nmea;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;

using namespace std;
using namespace chrono_literals;
using namespace Eigen;
using namespace nlohmann;

GpsDevice::GpsDevice(const string ns) : 
    VariableManager(ns, 2ms), 
    gpsfound(false)
{
}
 

void GpsDevice::init() {
    // set raw state to zero
    State rawState(Vector3d::Zero(), Vector3d::Zero(), Vector3d::Zero(), Vector3d::Zero());
    platform.updateState(rawState.asAffine());
    setRedisJsonStates(platform, rawState);

    // Connect to gps platform
    if (platform.gps.device.compare("socket") == 0) {
        LoggerStream::getInstance() << INFO << "Connecting to Stonex GPS on IP " << platform.gps.ip << " on port " << platform.gps.udp_port;
        peripheral = make_unique<Stonex>(platform.gps.ip, platform.gps.udp_port);
    } else if (platform.gps.device.compare("serial") == 0) {
        LoggerStream::getInstance() << INFO << "Connecting to Simplertk3b GPS on serial port " << platform.gps.usb_port;
        NtripCredentials credentials(
            platform.gps.ntrip_server,
            2101,
            platform.gps.ntrip_uname,
            platform.gps.ntrip_pwd,
            platform.gps.ntrip_mountpoint
        );
        peripheral = make_unique<Simplertk3b>(platform.gps.usb_port, credentials);
    } else {
        LoggerStream::getInstance() << ERROR << "Variable platform.gps.device must be 'socket' or 'serial', but was " << platform.gps.device;
        throw std::runtime_error("Variable platform.gps.device must be 'socket' or 'serial', but was " + platform.gps.device);
    }

    peripheral->start();
}

void GpsDevice::serverTick() {
    if (!peripheral->loaded) return;  // break if peripheral is not loaded yet

    // extract and update values
    shared_ptr<NmeaMessagePack> lines = peripheral->getNmeaLines(); // copy nmealine
    // GGA line
    double lat = lines->gga->getValue<double>("lat");
    double lng = lines->gga->getValue<double>("lon");

    double x, y;
    if (lat < -90 || lat > 90 || lng < -180 || lng > 180) {
        LoggerStream::getInstance() << INFO << "GPS out of bounds: lat=" << lat << ", lon=" << lng;
        lat = 0;
        lng = 0;
        x = 0;
        y = 0;
    } else {
        LatLonToUTMXY(lat, lng, platform.gps.utm_zone, x, y);
    }


    double height = lines->gga->getValue<double>("height");
    double fix = lines->gga->getValue<int>("fix");
    double time = lines->gga->getValue<long>("time");

    getVariable("pc.gps.fix")->setValue<int>(fix);

    // Update robot state
    rawT = Vector3d(x, y, height);
    // VTG line
    double gpsBaseLinearVelocity = lines->vtg->getValue<double>("ground_speed_km_per_h");
    getVariable("pc.gps.ground_speed")->setValue<double>(gpsBaseLinearVelocity);
    // HDT line
    // double yaw = constrainAngle(360 - lines->hdt->getValue<double>("heading") + platform.gps.antenna_rotation); 
    // rawR = Vector3d(rawR.x(), rawR.y(), yaw);
    // HRP line
    double yaw = constrainAngle(360 - lines->hrp->getValue<double>("heading") + platform.gps.antenna_rotation); 
    // Slope mode 0.0 -> auto, 1.0 -> manual positive, -1.0 -> manual negative
    double slopeMode = getVariable("pc.gps.slope_mode")->getValue<double>();
    double slopeCorrection = (slopeMode == 0.0) ? -sgn(platform.gps.antenna_rotation) : slopeMode;
    // pitch is roll on robot
    double roll = slopeCorrection * lines->hrp->getValue<double>("pitch");  
    // double pitch = line->getValue<double>("roll");
    double pitch = 1e-6;  // make pitch very small otherwise the direction is unclear
    rawR = Vector3d(pitch, roll, yaw);

    double varYaw = pow(lines->hrp->getValue<double>("heading_deviation"), 2);
    double varRoll = pow(lines->hrp->getValue<double>("pitch_deviation"), 2);
    // double varPitch = pow(line->getValue<double>("roll_deviation"), 2);
    double varPitch = 0.0;
    rawRCov = Vector3d(varPitch, varRoll, varYaw);

    int hrp_mode = lines->hrp->getValue<int>("mode");
    getVariable("pc.gps.hrp_mode")->setValue<double>(hrp_mode);

    if ( !getVariable("pc.simulation.active")->getValue<bool>() ) {
        State rawState(rawT, rawR, rawTCov, rawRCov);
        Vector3d r = rawState.getR().asVector();
        Vector3d t = rawState.getT().asVector();

        // LoggerStream::getInstance() << INFO << "Gps Vector T(x, y, z): (" << rawT[0] << ", " << rawT[1] << ", " << rawT[2] << ")"; 
        // LoggerStream::getInstance() << INFO << "Raw Vector T(x, y, z): (" << t[0] << ", " << t[1] << ", " << t[2] << ")"; 
        // LoggerStream::getInstance() << INFO << "Gps Vector R(r, p, y): (" << rawR[0] << ", " << rawR[1] << ", " << rawR[2] << ")"; 
        // LoggerStream::getInstance() << INFO << "Raw Vector R(r, p, y): (" << r[0] << ", " << r[1] << ", " << r[2] << ")"; 
        // LoggerStream::getInstance() << INFO << " --- ";


        platform.updateState(rawState.asAffine());
        setRedisJsonStates(platform, rawState);
    }

}

int main() {
    // First check if ILVO_PATH environment variable is set
    if (getenv("ILVO_PATH") == NULL) { 
        throw EnvVariableNotFoundException("$ILVO_PATH");
    }

    string procName = "ilvo-gps";
    LoggerStream::createInstance(procName);
    LoggerStream::getInstance() << INFO << "GpsDevice Logger started";
    GpsDevice gps(procName);
    gps.run();
    return 0;    
}
