#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_nmea_test

#include <boost/test/included/unit_test.hpp>
#include <string>
#include <vector>

#include <math.h>
#include <Utils/Settings/Traject.h>
#include <Utils/Geometry/Point.h>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Logging;

using namespace std;

// TODO export ILVO_PATH=/home/awillekens/Documents/robitics/sw-stack/src/UnitTest/files/robot

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE( TrajectTest )

const string pathPrefix = "/home/awillekens/Documents/robitics/test-arc/";

void logInterpolation(string filename, const vector<PointPtr>& points){
    ofstream outfile(pathPrefix + "traject/" + filename);

    outfile << "x,y" << endl;
    for (int i = 0; i < points.size(); i++) {
        outfile << *points[i] << endl;
    }   

    outfile.close();
}

void logCorners(string filename, const vector<CornerPointPtr>& corners){
    ofstream outfile(pathPrefix + "traject/" + filename);

    outfile << "cornerIndex,x,y,angle,pathIndex,headland" << endl;
    for (int i = 0; i < corners.size(); i++) {
        outfile << *corners[i] << endl;
    }

    outfile.close();
}

BOOST_AUTO_TEST_CASE( TrajectExamle )
{
    // Arrange
    LoggerStream::createInstance("traject-test", true);
    Traject t;

    // Act
    t.load("example", 31, 15.0, 0.1, 2.0);

    // Assert
    logCorners("example-corners.csv", t.getCorners());
    logInterpolation("example-interpolation.csv", t.getInterpolation(InterpolationType::LINEAR));
    logInterpolation("example-interpolation-curvy.csv", t.getInterpolation(InterpolationType::CURVY));
}

BOOST_AUTO_TEST_CASE( TrajectPenetrometerS15 )
{
    // Arrange
    LoggerStream::createInstance("traject-test", true);
    Traject t;

    // Act
    t.load("Penetrometer_s15", 31, 15.0, 0.1, 2.0);

    // Assert
    logCorners("penetrometer-s15-corners.csv", t.getCorners());
    logInterpolation("penetrometer-s15-interpolation.csv", t.getInterpolation(InterpolationType::LINEAR));
    logInterpolation("penetrometer-s15-interpolation-curvy.csv", t.getInterpolation(InterpolationType::CURVY));
}

BOOST_AUTO_TEST_CASE( TrajectPenetrometerBlok3 )
{
    // Arrange
    LoggerStream::createInstance("traject-test", true);
    Traject t;

    // Act
    t.load("blok3", 31, 15.0, 0.1, 2.0);

    // Assert
    logCorners("blok3-corners.csv", t.getCorners());
    logInterpolation("blok3-interpolation.csv", t.getInterpolation(InterpolationType::LINEAR));
    logInterpolation("blok3-interpolation-curvy.csv", t.getInterpolation(InterpolationType::CURVY));
}


BOOST_AUTO_TEST_SUITE_END()
