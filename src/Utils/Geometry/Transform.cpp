#include <Utils/Geometry/Transform.h>
#include <Utils/Geometry/Angle.h>
#include <ThirdParty/Eigen/Geometry> 

using namespace std;
using namespace Eigen;
using namespace nlohmann;

Vector3d Ilvo::Utils::Geometry::stdToEigen(vector<double>& v)
{
    return Vector3d(v[0], v[1], v[2]);
}


Vector3d Ilvo::Utils::Geometry::rotationToEuler(Eigen::Matrix3d& r)
{
        Vector3d euler;
        euler(0) = radToDeg(atan2(r(2, 1), r(2, 2)));
        euler(1) = radToDeg(atan2(-r(2, 0), sqrt(r(2, 1) * r(2, 1) + r(2, 2) * r(2, 2))));
        euler(2) = radToDeg(atan2(r(1, 0), r(0, 0)));

        euler(1) = calcSmallestAngle(euler(1), 0.0);  // constraint roll [-180.0, 180.0]
        
        return euler;
}


vector<double> Ilvo::Utils::Geometry::eigenToStd(Vector3d& v)
{
    return vector<double>({v.x(), v.y(), v.z()});
}

double Ilvo::Utils::Geometry::radToDeg(double value)
{
    return (value * 180.0f / M_PI);
}
double Ilvo::Utils::Geometry::degToRad(double value)
{
    return (value * M_PI / 180.0f);
}

double Ilvo::Utils::Geometry::toRobotFrame(double x){
    x = constrainAngle(x - 90.0);
    return x;
}

// vectorToAffine
Affine3d Ilvo::Utils::Geometry::vectorToAffine(Vector3d t, Vector3d r, bool inDegrees)
{
    Affine3d r_ = vectorToRotationAffine(r, inDegrees);
    Affine3d t_ = vectorToTranslationAffine(t);
    return (Affine3d) t_ * r_;
}

void Ilvo::Utils::Geometry::vectorToAffine(Affine3d& m, Vector3d t, Vector3d r, bool inDegrees)
{
    m = vectorToAffine(t, r, inDegrees);
}

Affine3d Ilvo::Utils::Geometry::vectorToAffine(std::vector<double> t, std::vector<double> r, bool inDegrees) 
{
    return vectorToAffine(stdToEigen(t), stdToEigen(r), inDegrees);
}

Affine3d Ilvo::Utils::Geometry::vectorToTranslationAffine(Vector3d v)
{
    return (Affine3d) Translation3d(v);
}

Affine3d Ilvo::Utils::Geometry::vectorToRotationAffine(Vector3d v, bool inDegrees)
{ 
    // https://stackoverflow.com/questions/31589901/euler-to-quaternion-quaternion-to-euler-using-eigen
    
    // In the robot coordinate axis: roll is around the y-axis, pitch around the x-axis and yaw around the z-axis
    double roll, pitch, yaw;
    if (inDegrees) {
        pitch = degToRad(v.x()); 
        roll = DegToRad(v.y()); 
        yaw = DegToRad(v.z()); 
    } else {
        pitch = v.x();
        roll = v.y();
        yaw = v.z();
    }

    Quaterniond q = AngleAxisd(yaw, Vector3d::UnitZ())       // will first apply yaw
                    * AngleAxisd(roll, Vector3d::UnitY())    // will second apply roll (our roll is around the y-axis)    
                    * AngleAxisd(pitch, Vector3d::UnitX());   

    return Affine3d(q.toRotationMatrix());
}

Affine3d Ilvo::Utils::Geometry::vectorToTranslationAffine(vector<double> v)
{
    return vectorToTranslationAffine(stdToEigen(v));
}

Affine3d Ilvo::Utils::Geometry::vectorToRotationAffine(vector<double> v, bool inDegrees)
{
    return vectorToRotationAffine(stdToEigen(v), inDegrees);
}


void Ilvo::Utils::Geometry::affineToVectors(Vector3d& t, Vector3d& r, Affine3d m)
{
    t = m.translation();
    Matrix3d r_m = m.rotation();
    r = rotationToEuler(r_m);
}

void Ilvo::Utils::Geometry::affineToVectors(vector<double>& t, vector<double>& r, Affine3d m)
{
    Vector3d t_;
    Vector3d r_;
    affineToVectors(t_, r_, m);
    t = eigenToStd(t_);
    r = eigenToStd(r_);
}  

json Ilvo::Utils::Geometry::matrixToJson(Matrix3d m)
{
    json rows = json::array();
    for (int i=0; i < m.rows(); i++) {
        json col = json::array();
        for (int j=0; j < m.cols(); j++){
            col.push_back(m.coeff(i,j));
        }
        rows.push_back(col);
    }
    return rows; 
}

Matrix3d Ilvo::Utils::Geometry::jsonToMatrix(json j_)
{
    int numCols = j_[0].size();
    int numRows = j_.size();
    Matrix3d m(numRows, numCols);
    for (int i=0; i < m.rows(); i++) {
        for (int j=0; j < m.cols(); j++){
            m(i,j) = (double) j_[i][j];
        }
    }
    return m;  
}

Affine3d Ilvo::Utils::Geometry::calculateHingeTransform(double linkLength, double angle)
{
    Vector3d t(0.0, -linkLength*cos(DegToRad(-angle)), linkLength*sin(DegToRad(-angle)));
    Vector3d r(0.0, 0.0, 0.0);
    Affine3d m;
    vectorToAffine(m, t, r);
    return m;  
}
