#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_test_navbase

#include <boost/test/included/unit_test.hpp>
#include <Utils/Settings/Platform.h>
#include <Utils/Logging/LoggerStream.h>
#include <math.h>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Logging;

using namespace std;
using namespace nlohmann;

/**
 * @brief Make sure the config_test.yaml file is in the correct working directory.
 * 
 */

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE( PlatformSettings )

BOOST_AUTO_TEST_CASE( readout )
{
    // Arrange
    string settings_path = string(getenv("TEST_ILVO_PATH")) + "/robot/settings.json";
    std::ifstream ifs(settings_path);
    json jf = json::parse(ifs);
    // Initialize logger
    LoggerStream::createInstance("test", true);

    // Act
    Ilvo::Utils::Settings::Platform& platform = Ilvo::Utils::Settings::Platform::getInstance();

    stringstream ss;
    ss << platform;

    string js_orig = jf.dump();
    string js_out = ss.str();
    json jo = json::parse(js_out);

    cout << "--ORIGINAL FILE" << endl;
    cout << js_orig << endl;
    cout << "--NEW COMPOSED FILE" << endl;
    cout << js_out << endl;

    // Assert
    BOOST_TEST(jf["name"] == jo["name"]);

}

BOOST_AUTO_TEST_SUITE_END()