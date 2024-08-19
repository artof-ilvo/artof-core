/**
 * @file Arc.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Functions for arc operations
 * @version 0.1
 * @date 2024-05-03
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Geometry/Point.h>
#include <Utils/Geometry/Line.h>


namespace Ilvo {
namespace Utils {
namespace Geometry {

    class Arc
    {
    private:
        /* data */
    public:
        Arc() = default;
        Arc(Point pointStart, Point pointStop, Point pointOtherSide, double radius);
        Arc(Line line1, Line line2, double radius);
        ~Arc() = default;

        Point pointCenter;
        Point pointStart;
        Point pointStop;

        double radius;
        bool major;

        std::vector<PointPtr> interpolate(double interpolationDistance) const;

    };

    inline std::ostream& operator<<(std::ostream& os, const Arc& arc) {
        os << "x,y,label" << std::endl;
        os << arc.pointCenter << ",center" << std::endl;
        os << arc.pointStart << ",start" << std::endl;
        os << arc.pointStop << ",stop" << std::endl;

        for (auto p : arc.interpolate(0.1)) {
            os << *p << ",point" << std::endl;
        }

        return os;
    }

} // namespace Geometry
} // namespace Utils
} // namespace Ilvo