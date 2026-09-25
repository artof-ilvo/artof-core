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
        double stepTowards(double current, double target, double maxStep);
    public:
        VelocityControl(double maxLinearAcc, double maxAngularAcc);
        ~VelocityControl() = default;

        void update(double &lonVel, double &latVel, double &omega, double dt_secs);
    };

}
}