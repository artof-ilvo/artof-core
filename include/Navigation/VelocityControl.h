namespace Ilvo {
namespace Core {

    class VelocityControl
    {
    private:
        double maxLinearAcc;  // m/s^2
        double maxAngularAcc; // rad/s^2

        double lonVelMem = 0.0;
        double latVelMem = 0.0;
        double angVelMem = 0.0;

        double sign(double x) { return (x > 0) - (x < 0); }
    public:
        VelocityControl(double maxLinearAcc, double maxAngularAcc);
        ~VelocityControl() = default;

        void update(double targetLon, double targetLat, double targetOmega, 
                                    double &outLon, double &outLat, double &outOmega, 
                                    double dt_secs);
    };

}
}