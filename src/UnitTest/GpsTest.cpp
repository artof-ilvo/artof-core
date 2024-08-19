#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_nmea_test

#include <boost/test/included/unit_test.hpp>
#include <string>
#include <vector>

#include <math.h>
#include <Utils/Nmea/Nmea.h>

using namespace Ilvo::Utils::Nmea;

using namespace std;

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE(Nmealines)

BOOST_AUTO_TEST_CASE( ggaTest1 )
{
    // Arrange
    std::string ggaStr = "$GPGGA,134658.00,5106.9792,N,11402.3003,W,2,09,1.0,1048.47,M,-16.27,M,08,AAAA*60";
    vector<char> ggaVec = vector<char>(ggaStr.begin(), ggaStr.end());
    // Act
    NmeaLine nmea(ggaVec);
    std::cout << nmea << std::endl;
    // Assert
    BOOST_TEST(nmea.getValue<int>("fix") == 2);
    BOOST_TEST((floor(nmea.getValue<double>("lat")*10000)/10000) == (std::floor(51.1163*10000)/10000));
    BOOST_TEST((floor(nmea.getValue<double>("lon")*10000)/10000) == (std::floor(114.038338333*10000)/10000));
    BOOST_TEST(nmea.getValue<double>("height") == 1048.47);
}

BOOST_AUTO_TEST_CASE( ggaTest2 )
{
    // Arrange
    std::string ggaStr = "$GPGGA,084310.70,5058.9727940,N,00346.7146948,E,5,06,9.9,80.300,M,0.00,M,06,2069*59";
    vector<char> ggaVec = vector<char>(ggaStr.begin(), ggaStr.end());
    // Act
    NmeaLine nmea(ggaVec);
    std::cout << nmea << std::endl;
    // Assert
    BOOST_TEST(nmea.getValue<int>("fix") == 5);
    BOOST_TEST((std::floor(nmea.getValue<double>("lat")*10000)/10000) == (std::floor(50.9828799*10000)/10000));
    BOOST_TEST((std::floor(nmea.getValue<double>("lon")*10000)/10000) == (std::floor(3.778578247*10000)/10000));
    BOOST_TEST(nmea.getValue<double>("height") == 80.300);
}

BOOST_AUTO_TEST_CASE( hdtTest1 )
{
    // Arrange
    std::string hdtStr = "$GPHDT,123.456,T*32";
    vector<char> hdtVec = vector<char>(hdtStr.begin(), hdtStr.end());
    // Act
    NmeaLine nmea(hdtVec);
    std::cout << nmea << std::endl;
    // Assert
    BOOST_TEST(nmea.getValue<double>("heading") == 123.456);
}

BOOST_AUTO_TEST_CASE( hdtTest2 )
{
    // Arrange
    std::string hdtStr = "$GPHDT,,T*1B";
    vector<char> hdtVec = vector<char>(hdtStr.begin(), hdtStr.end());
    // Act
    NmeaLine nmea(hdtVec);
    std::cout << nmea << std::endl;
    // Assert
    BOOST_TEST(nmea.getValue<double>("heading") == 0);
}


BOOST_AUTO_TEST_SUITE_END()
