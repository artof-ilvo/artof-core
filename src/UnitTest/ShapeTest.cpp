#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_test_csv
#include <boost/test/included/unit_test.hpp>
#include <string>
#include <math.h>

#include <Utils/File/PointShapeFile.h>

using namespace std;
using namespace Ilvo::Utils::File;
using namespace Ilvo::Utils::Geometry;

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE(ShapeFiles)


BOOST_AUTO_TEST_CASE( testMultiPoint )
{
    // Arrange
    string filename = std::string(getenv("TEST_ILVO_PATH")) + "/testmultipoint/testmultipoint.shp";
    bool polygon = true;

    PointShapeFile f(polygon, 31);
    // Act
    f.init(filename);
    // Assert
    std::cout << "testMultiPoint" << std::endl;
    std::cout << "Polygon: " << (polygon ? "true" : "false") << std::endl;
    std::cout << f << std::endl;
    BOOST_TEST(polygon == f.isPolygon(0));
}

BOOST_AUTO_TEST_CASE( testMultiPolygon )
{
    // Arrange
    string filename = std::string(getenv("TEST_ILVO_PATH")) + "/testmultipolygon/testmultipolygon.shp";
    bool polygon = false;

    PointShapeFile f(polygon, 31);
    // Act
    f.init(filename);
    // Assert
    std::cout << "testMultiPolygon" << std::endl;
    std::cout << "Polygon: " << (polygon ? "true" : "false") << std::endl;
    std::cout << f << std::endl;
    BOOST_TEST(true == f.isPolygon(0));
}

BOOST_AUTO_TEST_CASE( testPolygon )
{
    // Arrange
    string filename = std::string(getenv("TEST_ILVO_PATH")) + "/testpolygon/testpolygon.shp";
    bool polygon = true;

    PointShapeFile f(polygon, 31);
    // Act
    f.init(filename);
    // Assert
    std::cout << "testPolygon" << std::endl;
    std::cout << "Polygon: " << (polygon ? "true" : "false") << std::endl;
    std::cout << f << std::endl;
    BOOST_TEST(true == f.isPolygon(0));
}


BOOST_AUTO_TEST_CASE( testLine )
{
    // Arrange
    string filename = std::string(getenv("TEST_ILVO_PATH")) + "/testline/testline.shp";
    bool polygon = false;

    PointShapeFile f(polygon, 31);
    // Act
    f.init(filename);
    // Assert
    std::cout << "testPolygon" << std::endl;
    std::cout << "Polygon: " << (polygon ? "true" : "false") << std::endl;
    std::cout << f << std::endl;
    BOOST_TEST(polygon == f.isPolygon(0));
}

BOOST_AUTO_TEST_SUITE_END()