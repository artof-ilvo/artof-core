/**
 * @file Transform.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Functionality on transformations
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <typeinfo>       // std::bad_cast
#include <iostream>
#include <float.h>
#include <stdexcept> // std::runtime_error
#include <memory>
#include <ThirdParty/UTM.hpp>
#include <ThirdParty/Eigen/Geometry> 
#include <vector>
#include <ThirdParty/json.hpp>


namespace Ilvo {
namespace Utils {
namespace Geometry {
    Eigen::Vector3d stdToEigen(std::vector<double>&);
    std::vector<double> eigenToStd(Eigen::Vector3d&);
    Eigen::Vector3d rotationToEuler(Eigen::Matrix3d&);

    double toRobotFrame(double x);
    void vectorToAffine(Eigen::Affine3d& m, Eigen::Vector3d t, Eigen::Vector3d r, bool inDegrees=true);
    Eigen::Affine3d vectorToAffine(Eigen::Vector3d t, Eigen::Vector3d r, bool inDegrees=true);
    Eigen::Affine3d vectorToAffine(std::vector<double> t, std::vector<double> r, bool inDegrees=true);
    Eigen::Affine3d vectorToTranslationAffine(Eigen::Vector3d v);
    Eigen::Affine3d vectorToTranslationAffine(std::vector<double> v);
    Eigen::Affine3d calculateHingeTransform(double linkLength, double angle);
    Eigen::Affine3d vectorToRotationAffine(Eigen::Vector3d v, bool inDegrees=true);
    Eigen::Affine3d vectorToRotationAffine(std::vector<double> v, bool inDegrees=true);
    void affineToVectors(Eigen::Vector3d& t, Eigen::Vector3d& r, Eigen::Affine3d m);
    void affineToVectors(std::vector<double>& t, std::vector<double>& r, Eigen::Affine3d m);
    nlohmann::json matrixToJson(Eigen::Matrix3d m);
    Eigen::Matrix3d jsonToMatrix(nlohmann::json j);

    double radToDeg(double);
    double degToRad(double);
} // namespace Ilvo
} // namespace Utils
} // namespace Geometry
