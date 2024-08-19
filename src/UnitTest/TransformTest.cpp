#define BOOST_TEST_DYN_LINK 
#define BOOST_TEST_MODULE boost_test_navbase

#include <boost/test/included/unit_test.hpp>
#include <Utils/Settings/Section.h>
#include <Utils/Settings/Hitch.h>
#include <Utils/Settings/Robot.h>
#include <Utils/Settings/Gps.h>
#include <ThirdParty/Eigen/Geometry>
#include <math.h>
#include <fstream>

using namespace Ilvo::Utils::Settings;

using namespace std;
using namespace nlohmann;
using namespace Eigen;

/**
 * @brief Make sure the config_test.yaml file is in the correct working directory.
 * 
 */

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE( PlatformSettings )

BOOST_AUTO_TEST_CASE( transform1 )
{
    // Arrange
    std::ifstream ifs_robot(std::string(getenv("TEST_ILVO_PATH")) + "/testtransform/robot.json");
    json j_robot = json::parse(ifs_robot);

    std::ifstream ifs_states(std::string(getenv("TEST_ILVO_PATH")) + "/testtransform/states.json");
    json jStates = json::parse(ifs_states);

    Gps gps(j_robot["gps"]);
    Robot robot(j_robot["robot"]);

    std::vector<Hitch> hitches;
    for (json j_hitch: j_robot["hitches"]) {
        hitches.push_back(Hitch(j_hitch));
    }

    std::vector<shared_ptr<Section>> sections;
    for (json j_section: j_robot["sections"]) {
        sections.push_back(make_shared<Section>(j_section));
    }

    std::vector<json> states;
    for (json jState: jStates) {
        states.push_back(jState);
    }

    // Act
    json j_allTests = json::array();
    for (int s=0; s < states.size(); s++) {
        State rawState = State(states[s]["state"]);
        json j_testResult;
        gps.updateState(rawState.asAffine() * gps.getRefTransform());
        cout << "rawState" << endl;
        cout << rawState.asAffine().matrix() << endl;
        cout << "refTransform" << endl;
        cout << gps.getRefTransform().matrix() << endl;
        robot.updateState(gps.getState().asAffine() * robot.getRefTransform());
        j_testResult["rawState"]["state"] = rawState.toJson();
        j_testResult["gps"] = gps.toStateFullJson();
        j_testResult["robot"] = robot.toStateFullJson();

        for (int i=0; i < hitches.size(); i++) {
            Hitch hitch = hitches[i];
            double hitchAngle = states[s]["hitchAngles"][hitch.name];

            hitch.updateState(gps.getState().asAffine() * hitch.getRefTransform());
            json j_hitch = hitch.toStateFullJson(hitchAngle);
            
            for (int j=0; j < sections.size(); j++) {
                auto section = sections[j];
                double actualParallelAngle = 0.0;
                section->setParallelAngle(actualParallelAngle);
                section->updateState(hitch.getBallState(hitchAngle) * section->getParallelTransform());

                j_hitch["sections"].push_back(section->toStateFullJson());
            }
            j_testResult["hitches"].push_back(j_hitch);
        }
        j_allTests.push_back(j_testResult);
        cout << "--------" << endl;
    }

    std::ofstream ofs("result.json");
    ofs << j_allTests.dump(4) << std::endl;

    cout << "DONE!" << endl;
    // Assert
    // BOOST_TEST(js_orig.compare(js_out) == 0);

}

BOOST_AUTO_TEST_SUITE_END()