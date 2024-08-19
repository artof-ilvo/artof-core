/**
 * @file Angle.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Functionality to perform operations on angles in the traject or for the navigation algorithm
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

namespace Ilvo {
namespace Utils {
namespace Geometry {

    double calcSmallestAngle(double c1, double c2);
    double calcSmallestAngleAbsolute(double c1, double c2);
    double constrainAngle(double x);
    double atan2(double y, double x);
    int sgn(double val);
    double approx(double d, double dRef, double approxValue);
    bool inRange(double d1, double d2, double value);
    
} // namespace Ilvo
} // namespace Utils
} // namespace Geometry