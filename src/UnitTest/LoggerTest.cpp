#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_test_csv
#include <boost/test/included/unit_test.hpp>
#include <string>
#include <math.h>

#include <Utils/Logging/LoggerStream.h>

using namespace Ilvo::Utils::Logging;

using namespace std;

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE(LoggerTest)

BOOST_AUTO_TEST_CASE( steaming )
{
    // Arrange
    LoggerStream::createInstance("test-logger", true);

    // Act
    LoggerStream::getInstance() << INFO << "Hello!";
    LoggerStream::getInstance() << WARN << "Hello World!";
    LoggerStream::getInstance() << ERROR << "";
    LoggerStream::getInstance() << DEBUG << "Hello " << "World!";
    // Assert
    BOOST_TEST(1 == 1);
}

BOOST_AUTO_TEST_SUITE_END()
