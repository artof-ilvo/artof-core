/**
 * @file Geometry.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Functions for geometry operations
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once
#include <boost/geometry/geometries/point_xy.hpp>

namespace Ilvo {
namespace Utils {
namespace Geometry {
    class Point; // forward declaration
    typedef boost::geometry::model::d2::point_xy<double> bgPoint2D;

    class Geometry {
        public:
            virtual Point center() const = 0;
            virtual double distance(const Point&) const = 0;
    };
}
}
}