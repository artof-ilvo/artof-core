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
using namespace Ilvo::Utils::Geometry;

using namespace std;
using namespace nlohmann;
using namespace Eigen;

/**
 * @brief Make sure the config_test.yaml file is in the correct working directory.
 * 
 */

// Nmea lines test bench suite
BOOST_AUTO_TEST_SUITE( TransformTestRotationOrder )

Matrix3d calculateRy(vector<double> r) 
{
    double angleY = r[1];
    Matrix3d m {
        {cos(degToRad(angleY)), 0, sin(degToRad(angleY))},
        {0, 1, 0},
        {-sin(degToRad(angleY)), 0, cos(degToRad(angleY))}
    };
    return m;
}

Matrix3d calculateRz(vector<double> r)
{
    double angleZ = r[2];
    Matrix3d m {
        {cos(degToRad(angleZ)), -sin(degToRad(angleZ)), 0},
        {sin(degToRad(angleZ)), cos(degToRad(angleZ)), 0},
        {0, 0, 1}
    };
    return m;
}

Affine3d vectorToAffineVerify(vector<double> t, vector<double> r) 
{
    Matrix3d R = calculateRz(r) * calculateRy(r);
    Matrix4d M = Matrix4d::Identity();
    M.block<3,3>(0,0) = R;
    M(0,3) = t[0];
    M(1,3) = t[1];
    M(2,3) = t[2];
    return Affine3d(M);
}

BOOST_AUTO_TEST_CASE( test_rotation_matrix_seperate )
{
    // Arrange
    std::ifstream ifs_states(std::string(getenv("TEST_ILVO_PATH")) + "/testtransform/states.json");
    json jStates = json::parse(ifs_states);

    for (auto jState: jStates) {
        vector<double> r = jState["state"]["R"];

        Matrix3d Ry_verify = calculateRy(r);
        Matrix3d Ry_code = (Matrix3d) AngleAxisd(degToRad(r[1]), Vector3d::UnitY());
        cout << "Ry_verify ** Ry_code" << endl;
        cout << Ry_verify << endl;
        cout << "**" << endl;
        cout << Ry_code << endl;
        cout << "-----------------" << endl;
        BOOST_TEST(Ry_verify.isApprox(Ry_code));


        Matrix3d Rz_verify = calculateRz(r);
        Matrix3d Rz_code = (Matrix3d) AngleAxisd(degToRad(r[2]), Vector3d::UnitZ());
        cout << "Rz_verify ** Rz_code" << endl;
        cout << Rz_verify << endl;
        cout << "**" << endl;
        cout << Rz_code << endl;
        cout << "-----------------" << endl;
        BOOST_TEST(Rz_verify.isApprox(Rz_code));
    }

    cout << "DONE!" << endl;
}

BOOST_AUTO_TEST_CASE( test_vec_to_affine )
{
    // Arrange
    std::ifstream ifs_states(std::string(getenv("TEST_ILVO_PATH")) + "/testtransform/states.json");
    json jStates = json::parse(ifs_states);

    for (auto jState: jStates) {
        vector<double> t_orig = jState["state"]["T"];
        vector<double> r_orig = jState["state"]["R"];
        Vector3d t = stdToEigen(t_orig);
        Vector3d r = stdToEigen(r_orig);

        Affine3d a = vectorToAffine(t, r);
        Vector3d t_, r_;
        affineToVectors(t_, r_, a);
        cout << "calculated vectors" << endl;
        cout << t_.matrix() << endl << "--" << endl << r_.matrix() << endl;
        cout << "original vectors" << endl;
        cout << t.matrix() << endl << "--" << endl << r.matrix() << endl;
        cout << "-----------------" << endl;
        BOOST_TEST(t_.matrix().isApprox(t.matrix(), 0.1));
        BOOST_TEST(r_.matrix().isApprox(r.matrix(), 0.1));
    }

    cout << "DONE!" << endl;
}


BOOST_AUTO_TEST_CASE( test_affine_to_vec )
{
    // Arrange
    std::ifstream ifs_states(std::string(getenv("TEST_ILVO_PATH")) + "/testtransform/states.json");
    json jStates = json::parse(ifs_states);

    for (auto jState: jStates) {
        vector<double> r = jState["state"]["R"];
        vector<double> t = jState["state"]["T"];

        Affine3d a_verify = vectorToAffineVerify(t, r);
        Affine3d a_code = vectorToAffine(t, r);
        cout << "a_verify ** a_code" << endl;
        cout << a_verify.matrix() << endl;
        cout << "**" << endl;
        cout << a_code.matrix() << endl;
        cout << "-----------------" << endl;
        BOOST_TEST(a_verify.matrix().isApprox(a_code.matrix(), 0.001));
    }

    cout << "DONE!" << endl;
    // Assert
    // BOOST_TEST(js_orig.compare(js_out) == 0);

}

BOOST_AUTO_TEST_SUITE_END()