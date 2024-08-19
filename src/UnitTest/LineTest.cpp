#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_test_csv
#include <boost/test/included/unit_test.hpp>
#include <string>
#include <math.h>

#include <Utils/Geometry/Line.h>

using namespace std;
using namespace Ilvo::Utils::Geometry;

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE(LineTest)

const std::string pathPrefix = "/home/awillekens/Documents/robitics/test-arc/";


BOOST_AUTO_TEST_CASE( Extend1 )
{
    // Arrange
    Line line1(Point(0.0, 0.0), Point(1.0, 0.0));
    Line line2(Point(0.0, 0.0), Point(1.0, 0.0));
    Line line3(Point(0.0, 0.0), Point(1.0, 0.0));

    // Act
    line1.extend(0.5, Side::BEGIN);
    line2.extend(0.5, Side::END);
    line3.extend(0.5, Side::BOTH);

    // Assert
    BOOST_CHECK_CLOSE(line1.p1().x(), -0.5, 0.01);
    BOOST_CHECK_CLOSE(line1.p1().y(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line1.p2().x(), 1.0, 0.01);
    BOOST_CHECK_CLOSE(line1.p2().y(), 0.0, 0.01);

    BOOST_CHECK_CLOSE(line2.p1().x(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line2.p1().y(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line2.p2().x(), 1.5, 0.01);
    BOOST_CHECK_CLOSE(line2.p2().y(), 0.0, 0.01);

    BOOST_CHECK_CLOSE(line3.p1().x(), -0.5, 0.01);
    BOOST_CHECK_CLOSE(line3.p1().y(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line3.p2().x(), 1.5, 0.01);
    BOOST_CHECK_CLOSE(line3.p2().y(), 0.0, 0.01);
}

BOOST_AUTO_TEST_CASE( Extend2 )
{
    // Arrange
    Line line1(Point(0.0, 0.0), Point(0.0, 1.0));
    Line line2(Point(0.0, 0.0), Point(0.0, 1.0));
    Line line3(Point(0.0, 0.0), Point(0.0, 1.0));

    // Act
    line1.extend(0.5, Side::BEGIN);
    line2.extend(0.5, Side::END);
    line3.extend(0.5, Side::BOTH);

    // Assert
    // BOOST_CHECK_CLOSE has problems with very small numbers (happens for 90 degree lines)
    // So use small epsilon tests here
    BOOST_CHECK_SMALL(line1.p1().x() - 0.0, 0.01);
    BOOST_CHECK_SMALL(line1.p1().y() + 0.5, 0.01);
    BOOST_CHECK_SMALL(line1.p2().x() - 0.0, 0.01);
    BOOST_CHECK_SMALL(line1.p2().y() - 1.0, 0.01);

    BOOST_CHECK_SMALL(line2.p1().x() - 0.0, 0.01);
    BOOST_CHECK_SMALL(line2.p1().y() - 0.0, 0.01);
    BOOST_CHECK_SMALL(line2.p2().x() - 0.0, 0.01);
    BOOST_CHECK_SMALL(line2.p2().y() - 1.5, 0.01);

    BOOST_CHECK_SMALL(line3.p1().x() - 0.0, 0.01);
    BOOST_CHECK_SMALL(line3.p1().y() + 0.5, 0.01);
    BOOST_CHECK_SMALL(line3.p2().x() - 0.0, 0.01);
    BOOST_CHECK_SMALL(line3.p2().y() - 1.5, 0.01);
}

BOOST_AUTO_TEST_CASE( Extend3 ) 
{
    // Arrange
    Line line1(Point(0.0, 0.0), Point(1.0, 3.0));
    Line line2(Point(0.0, 0.0), Point(1.0, 3.0));
    Line line3(Point(0.0, 0.0), Point(1.0, 3.0));

    // Act
    double extention = 1.0;
    line1.extend(extention, Side::BEGIN);
    line2.extend(extention, Side::END);
    line3.extend(extention, Side::BOTH);

    // Assert
    BOOST_CHECK_CLOSE(line1.p1().x(), 0.0 - extention * cos(atan(3.0 / 1.0)) , 0.01);
    BOOST_CHECK_CLOSE(line1.p1().y(), 0.0 - extention * sin(atan(3.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line1.p2().x(), 1.0, 0.01);
    BOOST_CHECK_CLOSE(line1.p2().y(), 3.0, 0.01);

    BOOST_CHECK_CLOSE(line2.p1().x(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line2.p1().y(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line2.p2().x(), 1.0 + extention * cos(atan(3.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line2.p2().y(), 3.0 + extention * sin(atan(3.0 / 1.0)), 0.01);

    BOOST_CHECK_CLOSE(line3.p1().x(), 0.0 - extention * cos(atan(3.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line3.p1().y(), 0.0 - extention * sin(atan(3.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line3.p2().x(), 1.0 + extention * cos(atan(3.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line3.p2().y(), 3.0 + extention * sin(atan(3.0 / 1.0)), 0.01);
}

BOOST_AUTO_TEST_CASE( Extend4 )
{
    // Arrange
    Line line1(Point(0.5, 0.5), Point(-1.0, 3.0));
    Line line2(Point(0.5, 0.5), Point(-1.0, 3.0));
    Line line3(Point(0.5, 0.5), Point(-1.0, 3.0));

    // Act
    double extention = 1.0;
    line1.extend(extention, Side::BEGIN);
    line2.extend(extention, Side::END);
    line3.extend(extention, Side::BOTH);

    // Assert
    BOOST_CHECK_CLOSE(line1.p1().x(), 0.5 + extention * cos(atan(2.5 / -1.5)), 0.01);
    BOOST_CHECK_CLOSE(line1.p1().y(), 0.5 + extention * sin(atan(2.5 / -1.5)), 0.01);
    BOOST_CHECK_CLOSE(line1.p2().x(), -1.0, 0.01);
    BOOST_CHECK_CLOSE(line1.p2().y(), 3.0, 0.01);

    BOOST_CHECK_CLOSE(line2.p1().x(), 0.5, 0.01);
    BOOST_CHECK_CLOSE(line2.p1().y(), 0.5, 0.01);
    BOOST_CHECK_CLOSE(line2.p2().x(), -1.0 - extention * cos(atan(2.5 / -1.5)), 0.01);
    BOOST_CHECK_CLOSE(line2.p2().y(), 3.0 - extention * sin(atan(2.5 / -1.5)), 0.01);

    BOOST_CHECK_CLOSE(line3.p1().x(), 0.5 + extention * cos(atan(2.5 / -1.5)), 0.01);
    BOOST_CHECK_CLOSE(line3.p1().y(), 0.5 + extention * sin(atan(2.5 / -1.5)), 0.01);
    BOOST_CHECK_CLOSE(line3.p2().x(), -1.0 - extention * cos(atan(2.5 / -1.5)), 0.01);
    BOOST_CHECK_CLOSE(line3.p2().y(), 3.0 - extention * sin(atan(2.5 / -1.5)), 0.01);
}

BOOST_AUTO_TEST_CASE( Extend5 )
{
    // Arrange
    Line line1(Point(0.0, 0.0), Point(-1.0, -1.0));
    Line line2(Point(0.0, 0.0), Point(-1.0, -1.0));
    Line line3(Point(0.0, 0.0), Point(-1.0, -1.0));

    // Act
    double extention = 1.0;
    line1.extend(extention, Side::BEGIN);
    line2.extend(extention, Side::END);
    line3.extend(extention, Side::BOTH);

    // Assert
    BOOST_CHECK_CLOSE(line1.p1().x(), 0.0 + extention * cos(atan(1.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line1.p1().y(), 0.0 + extention * sin(atan(1.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line1.p2().x(), -1.0, 0.01);
    BOOST_CHECK_CLOSE(line1.p2().y(), -1.0, 0.01);

    BOOST_CHECK_CLOSE(line2.p1().x(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line2.p1().y(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line2.p2().x(), -1.0 - extention * cos(atan(1.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line2.p2().y(), -1.0 - extention * sin(atan(1.0 / 1.0)), 0.01);

    BOOST_CHECK_CLOSE(line3.p1().x(), 0.0 + extention * cos(atan(1.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line3.p1().y(), 0.0 + extention * sin(atan(1.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line3.p2().x(), -1.0 - extention * cos(atan(1.0 / 1.0)), 0.01);
    BOOST_CHECK_CLOSE(line3.p2().y(), -1.0 - extention * sin(atan(1.0 / 1.0)), 0.01);
}

BOOST_AUTO_TEST_CASE( Extend6 )
{
    // Arrange
    Line line1(Point(0.0, 0.0), Point(1.0, -1.0));
    Line line2(Point(0.0, 0.0), Point(1.0, -1.0));
    Line line3(Point(0.0, 0.0), Point(1.0, -1.0));

    // Act
    double extention = 1.0;
    line1.extend(extention, Side::BEGIN);
    line2.extend(extention, Side::END);
    line3.extend(extention, Side::BOTH);

    // Assert
    BOOST_CHECK_CLOSE(line1.p1().x(), 0.0 - extention * cos(atan(1.0 / -1.0)), 0.01);
    BOOST_CHECK_CLOSE(line1.p1().y(), 0.0 - extention * sin(atan(1.0 / -1.0)), 0.01);
    BOOST_CHECK_CLOSE(line1.p2().x(), 1.0, 0.01);
    BOOST_CHECK_CLOSE(line1.p2().y(), -1.0, 0.01);

    BOOST_CHECK_CLOSE(line2.p1().x(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line2.p1().y(), 0.0, 0.01);
    BOOST_CHECK_CLOSE(line2.p2().x(), 1.0 + extention * cos(atan(1.0 / -1.0)), 0.01);
    BOOST_CHECK_CLOSE(line2.p2().y(), -1.0 + extention * sin(atan(1.0 / -1.0)), 0.01);

    BOOST_CHECK_CLOSE(line3.p1().x(), 0.0 - extention * cos(atan(1.0 / -1.0)), 0.01);
    BOOST_CHECK_CLOSE(line3.p1().y(), 0.0 - extention * sin(atan(1.0 / -1.0)), 0.01);
    BOOST_CHECK_CLOSE(line3.p2().x(), 1.0 + extention * cos(atan(1.0 / -1.0)), 0.01); 
    BOOST_CHECK_CLOSE(line3.p2().y(), -1.0 + extention * sin(atan(1.0 / -1.0)), 0.01);
}

BOOST_AUTO_TEST_SUITE_END()
