#include <Utils/Geometry/Polygon.h>
#include <Utils/Geometry/Point.h>
#include <boost/geometry/algorithms/centroid.hpp>
#include <vector>
#include <ThirdParty/Eigen/Dense>

using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace nlohmann;
using namespace std;
using namespace Eigen;
namespace bg = boost::geometry;

Polygon::Polygon(vector<PointPtr> points)
{
    for (PointPtr& ptr: points) {
        bg::append(p, ptr->geometry());
    }
} 

Polygon::Polygon(vector<Point> points)
{
    for (Point& pt: points) {
        bg::append(p, pt.geometry());
    }
} 

Point Polygon::center() const
{
    bgPoint2D c;
    bg::centroid(this->p, c);
    return Point(c.x(), c.y());
}

double Polygon::distance(const Point& p) const
{
    Point c = center();
    return sqrt(pow(c.x() - p.x(), 2) + pow(c.y() - p.y(), 2));
}

const bgPolygon2D& Polygon::geometry() const
{
    return p;
}

json Polygon::toJson() const
{
    json j;
    j = json::array();

    for( const auto& point : p.outer() )
    {
        json j_point;
        double x = point.x();
        double y = point.y();
        j_point["x"] = x;
        j_point["y"] = y;
        j.push_back(j_point);
    }

    return j;
}

void Polygon::update(TransformMatrix transform, double width, double height)
{
    // Check the input validity
    if (width <= 0) {
        throw invalid_argument("Width must be positive");
    }
    if (height <= 0) {
        throw invalid_argument("Height must be set or up and down must be set");
    }

    // Define the points of the polygon
    MatrixXd points(5, 4);
    points << -width / 2, -height / 2, 0, 1.0,
              width / 2, -height / 2, 0, 1.0,
              width / 2, height / 2, 0, 1.0,
              -width / 2, height / 2, 0, 1.0,
              -width / 2, -height / 2, 0, 1.0;  // Closing the polygon by repeating the first point

    MatrixXd transformed_points = points * transform.matrix().transpose();

    // Create the polygon
    p.outer().clear();
    for (int i = 0; i < transformed_points.rows(); i++) {
        bg::append(p, bgPoint2D(transformed_points(i, 0), transformed_points(i, 1)));
    }
}

void Polygon::update(TransformMatrix transform, double width, double up, double down)
{
    // Check the input validity
    if (width <= 0) {
        throw invalid_argument("Width must be positive");
    }

    // Define the points of the polygon
    MatrixXd points(5, 4);
    points << -width / 2, down, 0, 1.0,
              width / 2, down, 0, 1.0,
              width / 2, up, 0, 1.0,
              -width / 2, up, 0, 1.0,
              -width / 2, down, 0, 1.0;  // Closing the polygon by repeating the first point

    MatrixXd transformed_points = points * transform.matrix().transpose();

    // Create the polygon
    p.outer().clear();
    for (int i = 0; i < transformed_points.rows(); i++) {
        bg::append(p, bgPoint2D(transformed_points(i, 0), transformed_points(i, 1)));
    }
}

void Polygon::contour(vector<vector<double>>& robotLatLng, vector<vector<double>>& robotXY, int zone) const {
    for (auto point: geometry().outer()) {
        robotXY.push_back({point.x(), point.y()});
        double lat, lng;
        if ((1 <= zone) && (zone <= 60)) {
            UTMXYToLatLon(point.x(), point.y(), zone, false, lat, lng);
            robotLatLng.push_back({RadToDeg(lat), RadToDeg(lng)});
        }
    }
}