#include <Utils/Geometry/Arc.h>
#include <Utils/Geometry/Line.h>
#include <Utils/Geometry/Angle.h>

using namespace Ilvo::Utils::Geometry;
using namespace std;

Arc::Arc(Point pointStart, Point pointStop, Point pointOtherSide, double radius) :
    pointStart(pointStart), pointStop(pointStop), radius(radius), major(true)
{
    if (pointStart.distance(pointStop) > 2*radius) {
        throw std::invalid_argument("Start and stop points may not be more than twice the radius of the arc.");
    }

    Line line(pointStart, pointStop);

    double startStopDistance = pointStart.distance(pointStop);
    Point center = line.center();

    double distance = sqrt(pow(radius, 2) - pow(startStopDistance / 2, 2));
    Line centerLine = line.orthogonal(center, distance, !line.left(pointOtherSide));
    pointCenter = centerLine.p2();
}

Arc::Arc(Line line1, Line line2, double radius):
    radius(radius), major(false)
{
    Point pIntersect = line1.intersection(line2);
    double alpha = DegToRad(line1.corner(line2));
    double d = radius / tan(alpha / 2);
    
    double line1AngleDegree = constrainAngle(line1.alpha());
    double line2AngleDegree = constrainAngle(line2.alpha() + 180); // Turn around 180 degrees (towards the corner)
    double line1Angle = DegToRad(line1AngleDegree);
    double line2Angle = DegToRad(line2AngleDegree);

    Point pCommon = line1.p2();
    Point pLine1(pCommon.x() - d * cos(line1Angle), pCommon.y() - d * sin(line1Angle));
    Point pLine2(pCommon.x() - d * cos(line2Angle), pCommon.y() - d * sin(line2Angle));

    Line orthogonal1 = line1.orthogonal(pLine1, 1, true);
    Line orthogonal2 = line2.orthogonal(pLine2, 1, true);
    
    pointCenter = orthogonal1.intersection(orthogonal2);

    pointStart = pLine1;
    pointStop = pLine2;
}

std::vector<PointPtr> Arc::interpolate(double interpolationDistance) const
{
    vector<PointPtr> arcPoints;

    Line startLine(pointCenter, pointStart);
    Line stopLine(pointCenter, pointStop);
    Line startStopLine(pointStart, pointStop);

    double sign = ((startStopLine.left(pointCenter) && major) || (!startStopLine.left(pointCenter) && !major)) ? -1 : 1;

    double dAlpha = RadToDeg(atan2(interpolationDistance, radius));  // This applies for small angles

    double startAngle = constrainAngle(startLine.alpha());
    double stopAngle = constrainAngle(stopLine.alpha());

    double alpha = startAngle;

    while (calcSmallestAngleAbsolute(alpha, stopAngle) > dAlpha) {
        double alphaRadians = DegToRad(alpha);
        double x2 = pointCenter.x() + radius * cos(alphaRadians);
        double y2 = pointCenter.y() + radius * sin(alphaRadians);
        arcPoints.push_back(make_shared<CurvyPoint>(Point(x2, y2), radius));
        alpha = alpha + sign * dAlpha;
    }
    // push back last point
    arcPoints.push_back(make_shared<CurvyPoint>(pointStop, radius));

    return arcPoints;
}