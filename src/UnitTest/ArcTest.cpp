#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_test_csv
#include <boost/test/included/unit_test.hpp>
#include <fstream>
#include <Utils/Geometry/Arc.h>

using namespace std;
using namespace Ilvo::Utils::Geometry;

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE(ArcTest)

const string pathPrefix = "/home/awillekens/Documents/robitics/test-arc/";

void logHeadlandArc(string filename, Arc& arc, Point& pref) {
    cout << "Arc info (radius: " << arc.radius << ", " << (arc.major ? "major" : "minor") << ")" << endl;
    ofstream outfile(pathPrefix + "headland/" + filename);
    outfile << arc;
    outfile << pref << ",pref" << endl;
    outfile << endl;
}

void logCornerArc(string filename, Arc& arc, Point& p1, Point& p2, Point& p3) {
    cout << "Arc info (radius: " << arc.radius << ", " << (arc.major ? "major" : "minor") << ")" << endl;
    ofstream outfile(pathPrefix + "corner/" + filename);
    outfile << arc;
    outfile << p1 << ",p1" << endl;
    outfile << p2 << ",p2" << endl;
    outfile << p3 << ",p3" << endl;
    outfile << endl;
}

BOOST_AUTO_TEST_CASE( Headland1 )
{
    // Arrange
    Point p1 = Point(0.5, 0.5);
    Point p2 = Point(1.5, 2.0);
    Point pref = Point(1.0, 0.0);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland1.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Headland2 )
{
    // Arrange
    Point p1 = Point(0.5, 0.5);
    Point p2 = Point(1.5, 2.0);
    Point pref = Point(1.0, 2.0);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland2.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Headland3 )
{
    // Arrange
    Point p1 = Point(0.5, 0.5);
    Point p2 = Point(1.5, 0.0);
    Point pref = Point(1.0, 0.0);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland3.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Headland4 )
{
    // Arrange
    Point p1 = Point(0.5, 0.5);
    Point p2 = Point(1.5, 0.0);
    Point pref = Point(1.0, 2.0);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland4.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Headland5 )
{
    // Arrange
    Point p1 = Point(0.5, 0.5);
    Point p2 = Point(1.5, 0.5);
    Point pref = Point(1.0, 0.0);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland5.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Headland6 )
{
    // Arrange
    Point p1 = Point(0.5, 0.5);
    Point p2 = Point(1.5, 0.5);
    Point pref = Point(1.0, 2.0);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland6.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Headland7 )
{
    // Arrange
    Point p1 = Point(0.0, 0.5);
    Point p2 = Point(0.0, 1.5);
    Point pref = Point(1.0, 1.0);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland7.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Headland8 )
{
    // Arrange
    Point p1 = Point(0.0, 0.5);
    Point p2 = Point(0.0, 1.5);
    Point pref = Point(-1.0, 1.0);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland8.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Headland9 )
{
    // Arrange
    Point p1 = Point(537322.727141227,5658512.36768627);
    Point p2 = Point(537321.903607,5658513.62139731);
    Point pref = Point(537305.048063725,5658500.75470294);
    Arc arc(p1, p2, pref, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logHeadlandArc("headland9.csv", arc, pref);

    BOOST_CHECK_GT(arc.pointCenter.distance(pref), arc.radius);
}

BOOST_AUTO_TEST_CASE( Corner1 )
{
    // Arrange
    Point p1 = Point(0.0, 0.0);
    Point p2 = Point(1.5, 1.5);
    Point p3 = Point(3.0, 0.0);
    Line line1(p1, p2);
    Line line2(p2, p3);
    Arc arc(line1, line2, 1.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logCornerArc("corner1.csv", arc, p1, p2, p3);
}

BOOST_AUTO_TEST_CASE( Corner2 )
{
    // Arrange
    Point p1 = Point(0.0, 0.0);
    Point p2 = Point(1.5, 1.5);
    Point p3 = Point(1.0, 5.0);
    Line line1(p1, p2);
    Line line2(p2, p3);
    Arc arc(line1, line2, 1.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logCornerArc("corner2.csv", arc, p1, p2, p3);
}

BOOST_AUTO_TEST_CASE( Corner3 )
{
    // Arrange
    Point p1 = Point(-3.0, -1.0);
    Point p2 = Point(-5, -1.5);
    Point p3 = Point(-4.0, 0.0);
    Line line1(p1, p2);
    Line line2(p2, p3);
    Arc arc(line1, line2, 1.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logCornerArc("corner3.csv", arc, p1, p2, p3);
}

BOOST_AUTO_TEST_CASE( Corner4 )
{
    // Arrange
    Point p1 = Point(0.0, -5.0);
    Point p2 = Point(-5.0, -5.0);
    Point p3 = Point(-3.0, 0.0);
    Line line1(p1, p2);
    Line line2(p2, p3);
    Arc arc(line1, line2, 3.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logCornerArc("corner4.csv", arc, p1, p2, p3);
}

BOOST_AUTO_TEST_CASE( Corner5 )
{
    // Arrange
    Point p1 = Point(7.0, 9.0);
    Point p2 = Point(10.0, 10.0);
    Point p3 = Point(9.0, 13.0);
    Line line1(p1, p2);
    Line line2(p2, p3);
    Arc arc(line1, line2, 2.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logCornerArc("corner5.csv", arc, p1, p2, p3);
}

BOOST_AUTO_TEST_CASE( Corner6 )
{
    // Arrange
     Point p1 = Point(7.0, 9.0);
    Point p2 = Point(10.0, 10.0);
    Point p3 = Point(11.0, 13.0);
    Line line1(p1, p2);
    Line line2(p2, p3);
    Arc arc(line1, line2, 1.0);

    // Act
    auto points = arc.interpolate(0.1);
    // Assert
    logCornerArc("corner6.csv", arc, p1, p2, p3);
}



BOOST_AUTO_TEST_SUITE_END()
