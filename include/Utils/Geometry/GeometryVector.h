/**
 * @file GeometryVector.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief A vector of geometries and some helper functions
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <math.h>
#include <memory>
#include <string>
#include <sstream>
#include <ThirdParty/Eigen/Dense>
#include <Utils/Geometry/Point.h>
#include <Utils/Geometry/Polygon.h>


namespace Ilvo {
namespace Utils {
namespace Geometry {

    template<class T>
    bool compareGeometry(T p1, T p2)
    {
        Point origin;
        return (p1->distance(origin) > p2->distance(origin));
    }

    template<class T>
    class GeometryVector : public std::vector<T> {
        private:
            int closestIdx = -1;
            Eigen::MatrixXd m;

        public:
            GeometryVector() = default;
            GeometryVector(std::vector<T> v) : std::vector<T>(v) 
            {
                sort(this->begin(), this->end(), compareGeometry<T>);
                m = Eigen::MatrixXd::Zero(v.size(), 2);
                for (int i = 0; i < v.size(); i++) {
                    T geometry = v[i];
                    Point center = geometry->center();
                    m(i,0) = center.x();
                    m(i,1) = center.y();
                }
            } 

            std::vector<T> nearby(Point& p, int windowSize=10)
            {
                int halfWindowSize = std::floor(windowSize / 2);

                int startIdx, endIdx;

                // search entire array to closest point if closestIdx is smaller than one.
                if (closestIdx < 0) {
                    closestIdx = 0;
                    startIdx = 0;
                    endIdx = this->size() - 1;
                } else {
                    startIdx = std::max(closestIdx - halfWindowSize, 0);
                    endIdx = std::min(closestIdx + halfWindowSize, (int)(this->size() - 1));
                }

                // search closest point
                for (int i = startIdx; i <= endIdx; i++) {
                    T geometry = this->at(i);
                    T closestGeometry = this->at(closestIdx);
                    if (geometry->distance(p) < closestGeometry->distance(p)) {
                        closestIdx = i;
                    }
                }

                startIdx = std::max({closestIdx - halfWindowSize, 0});
                endIdx = std::min({closestIdx + halfWindowSize, (int)(this->size() - 1)});

                return std::vector<T>(this->begin() + startIdx, this->begin() + endIdx);
            }   

            // std::vector<PointPtr> nearbyDistance(Point& p, double distance=5.0)
            // {
            //     Eigen::MatrixXd mX = Eigen::MatrixXd::Constant(this->size(), 1, p.x());
            //     Eigen::MatrixXd mY = Eigen::MatrixXd::Constant(this->size(), 1, p.y());
            //     Eigen::MatrixXd mMaxDistance = Eigen::MatrixXd::Constant(this->size(), 1, distance);
            //     Eigen::MatrixXd mDistance = (m.col(0) - mX).transpose() * (m.col(0) - mX) + (m.col(1) - mY).transpose() * (m.col(1) - mY);

            //     std::vector<double> mDistanceFiltered(mDistance.array());
            //     std::vector<PointPtr> nearbyPoints;


            //     return nearbyPoints;
            // }


            ~GeometryVector() = default;
    };

    typedef GeometryVector<PolygonPtr> PolygonVector;
    typedef GeometryVector<PointPtr> PointVector;

} // namespace Ilvo
} // namespace Utils
} // namespace Geometry

