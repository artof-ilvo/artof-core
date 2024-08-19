#include <Utils/Settings/State.h>
#include <Utils/Geometry/Transform.h>
#include <ThirdParty/UTM.hpp>

using namespace Eigen;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Geometry;
using namespace nlohmann;
using namespace std;

pAffine::pAffine() : 
    Affine3d(Affine3d::Identity())
{}

pAffine::pAffine(vector<double> v) : 
    Affine3d(vectorToTranslationAffine(v))
{}

pAffine::pAffine(Vector3d v) : 
    Affine3d(vectorToTranslationAffine(v))
{}

Vector3d pAffine::asVector() const
{
    Vector3d t, r;
    affineToVectors(t, r, *this);
    return t;
}

json pAffine::toJson() const
{
    Vector3d t, r;
    affineToVectors(t, r, *this);
    json j = json::array();
    j.push_back(t.x());
    j.push_back(t.y());
    j.push_back(t.z());
    return j;
}

rAffine::rAffine() : 
    Affine3d(Affine3d::Identity())
{}

rAffine::rAffine(vector<double> v) : 
    Affine3d(vectorToRotationAffine(v))
{}

rAffine::rAffine(Vector3d v) : 
    Affine3d(vectorToRotationAffine(v))
{}

Vector3d rAffine::asVector() const
{
    Vector3d t, r;
    affineToVectors(t, r, *this);
    return r;
}

json rAffine::toJson() const
{
    Vector3d t, r;
    affineToVectors(t, r, *this);
    json j = json::array();
    j.push_back(r.x());
    j.push_back(r.y());
    j.push_back(r.z());
    return j;
}

CovMat::CovMat() : 
    Matrix3d(Matrix3d::Identity())
{}

CovMat::CovMat(vector<double> varVector): 
    Matrix3d(Vector3d(varVector[0], varVector[1], varVector[2]).asDiagonal())
{}

CovMat::CovMat(Vector3d varVector): 
    Matrix3d(Vector3d(varVector.x(), varVector.y(), varVector.z()).asDiagonal())
{}

CovMat::CovMat(Eigen::Matrix3d covMat):
    Matrix3d(covMat)
{}

json CovMat::toJson() const
{
    return matrixToJson(*this); 
}

State::State() : 
    t_(pAffine(Vector3d(0.0, 0.0, 0.0))),
    r_(rAffine(Vector3d(0.0, 0.0, 0.0)))
{}

State::State(vector<double> t, vector<double> r) : 
    t_(pAffine(Vector3d(t[0], t[1], t[2]))),
    r_(rAffine(Vector3d(r[0], r[1], r[2]))) 
{}

State::State(Vector3d t, Vector3d r) : 
    t_(pAffine(t)),
    r_(rAffine(r)) 
{}

State::State(Vector3d t, Vector3d r, Vector3d tCov, Vector3d rCov) : 
    t_(pAffine(t)),
    r_(rAffine(r)),
    tCovMat_(CovMat(tCov)),
    rCovMat_(CovMat(tCov)) 
{}

State::State(nlohmann::json jState) :
    t_(pAffine((vector<double>) jState["T"])),
    r_(rAffine((vector<double>) jState["R"])) 
{
    if (jState.contains("T_cov")) {
            tCovMat_ = jsonToMatrix(jState["T_cov"]);    
    }
    if (jState.contains("R_cov")) {
            rCovMat_ = jsonToMatrix(jState["R_cov"]);    
    }
}

State::State(Affine3d m)
{
    Vector3d t, r;
    affineToVectors(t, r, m);
    t_ = pAffine(t);
    r_ = rAffine(r);
}

json State::toJson(int zone) const
{
    json j;
    j["T"] = t_.toJson();
    j["R"] = r_.toJson();
    j["T_cov"] = tCovMat_.toJson();
    j["R_cov"] = tCovMat_.toJson();

    j["point"] = json();
    j["point"]["xy"] = xy();
    if ((1 <= zone) && (zone <= 60)) {
        j["point"]["latlng"] = latlng(zone);
    } else {
        j["point"]["latlng"] = {0.0, 0.0};
    }
    return j;
}

void State::setRCovMat(CovMat r)
{
    this->rCovMat_ = r;
}

void State::setTCovMat(CovMat t)
{
    this->tCovMat_ = t;
}

Affine3d State::asAffine()
{
    return t_ * r_;
}

const rAffine& State::getR() const
{
    return r_;
}
const pAffine& State::getT() const
{
    return t_;
}

void State::setR(Eigen::Vector3d r)
{
    r_ = rAffine(r);
}

void State::setT(Eigen::Vector3d t)
{
    t_ = pAffine(t);
}

CovMat& State::getRCovMat()
{
    return this->rCovMat_;
}

CovMat& State::getTCovMat()
{
    return this->tCovMat_;
}

vector<double> State::xy() const
{
    return {getT().asVector()[0], getT().asVector()[1]};
}

vector<double> State::latlng(int zone) const
{
    double lat, lng;
    vector<double> xy_ = xy();
    UTMXYToLatLon(xy_[0], xy_[1], zone, false, lat, lng);
    return {RadToDeg(lat), RadToDeg(lng)};
}

double State::heading()
{
    return getR().asVector()[2];
}

TransformMatrix::TransformMatrix() :
    Affine3d(Affine3d::Identity()) 
{}

TransformMatrix::TransformMatrix(json transform) : 
    Affine3d(vectorToAffine(
        (vector<double>) transform["T"],
        (vector<double>)  transform["R"]
    )) 
{}

TransformMatrix::TransformMatrix(Affine3d transform) :
    Affine3d(transform)
{}


TransformMatrix::TransformMatrix(Vector3d t, Vector3d r) :
    Affine3d(vectorToAffine(t, r))
{}

json TransformMatrix::toJson() const {
    json j;
    vector<double> t, r;
    affineToVectors(t, r, *this);
    j["T"] = t;
    j["R"] = r;
    return j;
}

StateFull::StateFull(json jTransform) :
    tRef(TransformMatrix(jTransform))
{}

StateFull::StateFull(Eigen::Affine3d tRef) :
    tRef(tRef)
{}


State& StateFull::updateState(Eigen::Affine3d m)
{
    currentState = State(m);
    return currentState;
}

State& StateFull::getState()
{
    return currentState;
}

Affine3d& StateFull::getRefTransform() 
{
    return tRef;
}

void StateFull::setRefTransform(json jTransform)
{
    tRef = TransformMatrix(jTransform);
}


json StateFull::toStateFullJson() const
{
    json j;
    j["state"] = currentState.toJson();
    return j;
}

json StateFull::toJson() const
{
    json j = prepareJson();
    j["transform"] = tRef.toJson();
    return j;
}