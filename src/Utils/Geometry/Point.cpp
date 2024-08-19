#include <Utils/Geometry/Point.h>

using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Redis;

using namespace std;
using namespace nlohmann;
using namespace Eigen;
namespace bg = boost::geometry;

Point::Point() : 
    p(0.0, 0.0), empty(true) 
{}

Point::Point(double x, double y) : 
    p(x, y), empty(false)
{}

Point::Point(shared_ptr<Point> pointPtr) :
    p(pointPtr->x(), pointPtr->y()), empty(false)
{}

Point::Point(Vector3d t) : 
    p(t.x(), t.y()), empty(false)
{}

Point::Point(Affine3d m) : 
    p(m.translation().x(), m.translation().y()), empty(false)
{}

const double Point::x() const
{
    return p.x();
}
const double Point::y() const
{
    return p.y();
}
void Point::x(double x)
{
    empty = false;
    p.x(x);
}
void Point::y(double y)
{
    empty = false;
    p.y(y);
}

bool Point::isEmpty() const
{
    return empty;
}

const bgPoint2D& Point::geometry() const
{
    return p;
}

Point::operator Eigen::Vector3d() 
{
    return Eigen::Vector3d(x(), y(), 0.0);
};


Point& Point::operator=(const Eigen::Vector3d& v)
{
    empty = false;
    p.x(v.x());
    p.y(v.y());
    return *this;
}

Point& Point::operator= (const Point& from)
{
    empty = false;
    p.x(from.x());
    p.y(from.y());
    return *this;
}

Point& Point::operator= (const Eigen::Affine3d& m)
{
    empty = false;
    p.x(m.translation().x());
    p.y(m.translation().y());
    return *this;
}

double Point::distance(const Point& p) const {
    return sqrt(pow(x() - p.x(), 2) + pow(y() - p.y(), 2));
}

Point Point::center() const
{
    return Point(this->x(), this->y());
}

json Point::toJson() const {
    json j;
    j["x"] = x();
    j["y"] = y();
    return j;
}

CurvyPoint::CurvyPoint() : Point(), radius(0.0) {}
CurvyPoint::CurvyPoint(const CurvyPoint& point) : Point(point.x(), point.y()), radius(point.radius) {}
CurvyPoint::CurvyPoint(const Point& point) : Point(point), radius(0.0) {}
CurvyPoint::CurvyPoint(double x, double y) : Point(x, y), radius(0.0) {}
CurvyPoint::CurvyPoint(const Point& point, double radius) : Point(point), radius(radius) {}
CurvyPoint::CurvyPoint(double x, double y, double radius) : Point(x, y), radius(radius) {}

CornerPoint::CornerPoint() : index(0), angle(0.0), cornerIndex(0), isHeadland(false), headlandDistance(0.0) {}
CornerPoint::CornerPoint(const CornerPoint& from) : index(from.index), point(from.point), angle(from.angle), cornerIndex(from.cornerIndex), isHeadland(from.isHeadland), headlandDistance(from.headlandDistance), previousRawPoint(from.previousRawPoint), nextRawPoint(from.nextRawPoint) {}
CornerPoint::CornerPoint(int index, Point point, double angle, int cornerIndex, Point previousRawPoint, Point nextRawPoint) : index(index), point(point), angle(angle), cornerIndex(cornerIndex), previousRawPoint(previousRawPoint), nextRawPoint(nextRawPoint), isHeadland(false) {}
void CornerPoint::setHeadland(double distance) { 
    isHeadland = true; 
    headlandDistance = distance;    
}

CornerPoint& CornerPoint::operator= (const CornerPoint& from)
{
    index = from.index;
    point = from.point;
    angle = from.angle;
    cornerIndex = from.cornerIndex;
    previousRawPoint = from.previousRawPoint;
    nextRawPoint = from.nextRawPoint;
    isHeadland = from.isHeadland;
    headlandDistance = from.headlandDistance;
    return *this;
}


NextAndPreviousCorner::NextAndPreviousCorner(CornerPoint previousCorner, CornerPoint nextCorner) : previousCorner(previousCorner), nextCorner(nextCorner) {}

IndexPoint::IndexPoint() : Point(), index(0) {}
IndexPoint::IndexPoint(Point point, int index) : Point(point), index(index) {}
IndexPoint::IndexPoint(double x, double y, int index) : Point(x, y), index(index) {}

IndexPoint& IndexPoint::operator= (const IndexPoint& from)
{
    empty = false;
    index = from.index;
    p.x(from.x());
    p.y(from.y());
    return *this;
}

IndexPoint& IndexPoint::operator= (const Point& from)
{
    empty = false;
    index = 0;
    p.x(from.x());
    p.y(from.y());
    return *this;
}

