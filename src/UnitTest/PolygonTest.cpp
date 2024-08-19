#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_test_csv
#include <boost/test/included/unit_test.hpp>
#include <string>
#include <math.h>

#include <Utils/Geometry/Polygon.h>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

using namespace std;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace boost::geometry;

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE(PolygonTest)

BOOST_AUTO_TEST_CASE( polygon1 )
{
    // Arrange
    vector<PointPtr> pointdata = vector<PointPtr>();
    pointdata.push_back(std::make_shared<Point>(0.0, 0.0));
    pointdata.push_back(std::make_shared<Point>(0.0, 1.0));
    pointdata.push_back(std::make_shared<Point>(1.0, 1.0));
    pointdata.push_back(std::make_shared<Point>(1.0, 0.0));
    pointdata.push_back(std::make_shared<Point>(0.0, 0.0));

    Point point1 = Point(0.5, 0.5);
    Point point2 = Point(1.5, 0.5);
    Polygon polygon(pointdata);
    // Act
    bool inside1 = covered_by(point1.geometry(), polygon.geometry());
    bool inside2 = covered_by(point2.geometry(), polygon.geometry());
    // Assert
    std::cout << polygon << std::endl;
    BOOST_TEST(inside1 == true);
    std::cout << "Point1 " << point1 << " " << (inside1 ? "IS" : "IS NOT") << " inside the polygon" << std::endl;
    BOOST_TEST(inside2 == false);
    std::cout << "Point2 " << point2 << " " << (inside2 ? "IS" : "IS NOT") << " inside the polygon" << std::endl;
}

BOOST_AUTO_TEST_CASE( polygon2 )
{
    // Arrange
    TransformMatrix transform({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0});
    Polygon polygon;

    // Act
    polygon.update(transform, 1.0, 2.0);
    // Assert
    std::cout << "Polygon2: " << std::endl;
    std::cout << polygon << std::endl;
}


BOOST_AUTO_TEST_SUITE_END()
