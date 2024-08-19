/**
 * @file State.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief State used for position and orientation description
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <vector>
#include <memory>
#include <Utils/Geometry/Transform.h>
#include <ThirdParty/json.hpp>
#include <ThirdParty/Eigen/Geometry>

namespace Ilvo {
namespace Utils {
namespace Settings {

class CovMat: public Eigen::Matrix3d {
    public:
        CovMat();
        CovMat(std::vector<double> varVector);
        CovMat(Eigen::Vector3d varVector);
        CovMat(nlohmann::json j_covMat);
        CovMat(Eigen::Matrix3d covMat);

        nlohmann::json toJson() const;
};

class pAffine: public Eigen::Affine3d {
    public:
        pAffine();
        pAffine(std::vector<double> v);
        pAffine(Eigen::Vector3d v);

        Eigen::Vector3d asVector() const;

        nlohmann::json toJson() const;
};

class rAffine: public Eigen::Affine3d {
    public:
        rAffine();
        rAffine(std::vector<double> v);
        rAffine(Eigen::Vector3d v);

        Eigen::Vector3d asVector() const;

        nlohmann::json toJson() const;
};


class State  {
    private:
        pAffine t_;
        rAffine r_;
        CovMat tCovMat_;
        CovMat rCovMat_;
    public:
        State();
        State(Eigen::Affine3d);
        State(Eigen::Vector3d t, Eigen::Vector3d r);
        State(Eigen::Vector3d t, Eigen::Vector3d r, Eigen::Vector3d covT, Eigen::Vector3d covR);
        State(std::vector<double> t, std::vector<double> r);
        State(nlohmann::json jState);
        ~State() = default;

        void setRCovMat(CovMat);
        void setTCovMat(CovMat);

        const rAffine& getR() const;
        const pAffine& getT() const;
        CovMat& getRCovMat();
        CovMat& getTCovMat();

        void setR(Eigen::Vector3d);
        void setT(Eigen::Vector3d);

        std::vector<double> xy() const;
        std::vector<double> latlng(int zone) const;
        double heading();

        Eigen::Affine3d  asAffine();

        nlohmann::json toJson(int zone=-1) const;
};


class TransformMatrix : public Eigen::Affine3d
{
public:
    TransformMatrix();
    TransformMatrix(nlohmann::json jTransform);
    TransformMatrix(Eigen::Affine3d transform);
    TransformMatrix(Eigen::Vector3d t, Eigen::Vector3d r);
    ~TransformMatrix() = default;
    
    nlohmann::json toJson() const;
};

class StateFull {
protected:
    State currentState;
    TransformMatrix tRef;
public:
    StateFull() = default;
    StateFull(nlohmann::json tRef);
    StateFull(Eigen::Affine3d transform);
    ~StateFull() = default;

    State& updateState(Eigen::Affine3d);
    State& getState();
    Eigen::Affine3d& getRefTransform();
    void setRefTransform(nlohmann::json jTransform);

    virtual nlohmann::json prepareJson() const = 0;
    nlohmann::json toJson() const;
    nlohmann::json toStateFullJson() const;
};

} // namespace Ilvo
} // namespace Utils
} // namespace Robot