#include <Utils/Geometry/Angle.h>

#include <math.h>

using namespace Ilvo::Utils::Geometry;
using namespace std;


/**
 * @brief Returns the smallest angle of c1 and c2
 * 
 * @param c1 First angle in degrees
 * @param c2 Second angle in degrees
 * @return double: Angle in degrees
 */
double Ilvo::Utils::Geometry::calcSmallestAngle(double c1, double c2) { 
    double a = c1 - c2;
    a += (a>180) ? -360 : (a<-180) ? 360 : 0;
    return a;
}

double Ilvo::Utils::Geometry::calcSmallestAngleAbsolute(double c1, double c2) { 
    double a1 = abs(c1 - c2);
    double a2 = 360 - abs(c1 - c2);
    return min(a1, a2);
}

/**
 * @brief Retruns an angle in [0, 360[
 * 
 * @param x 
 * @return double 
 */
double Ilvo::Utils::Geometry::constrainAngle(double x){
    x = fmod(x,360);
    if (x < 0)
        x += 360;
    return x;
}

/**
 * @brief Calculates the atan2 in [0, 2 Pi[
 * 
 * @param y 
 * @param x 
 * @return double: Angle in radian
 */
double Ilvo::Utils::Geometry::atan2(double y, double x) {
    double ret;
    if (x != 0) {
        if (x > 0) { 
            if (y >= 0) ret = atan(y/x); // 1e kwadrant 
            else ret = atan(y/x) + (2*M_PI); // 4e kwadrant
        } else { // x < 0: 2e kwadrant & 3e kwadrant
            ret = atan(y/x) + M_PI;
        } 
    } else { // x == 0
        if (y >= 0) ret = 0.5*M_PI; // 1e kwadrant 
        else ret = 1.5*M_PI; // 4e kwadrant 
    }
    return ret;
}

int Ilvo::Utils::Geometry::sgn(double val) {
    return (0 < val) - (val < 0);
}


double Ilvo::Utils::Geometry::approx(double d, double dRef, double approxValue)
{
    if (abs(d - dRef) <= approxValue) {
        return 0.0;
    } else {
        return d;
    }
}

bool Ilvo::Utils::Geometry::inRange(double d1, double d2, double value)
{
    return d1 <= value && value <= d2;
}
