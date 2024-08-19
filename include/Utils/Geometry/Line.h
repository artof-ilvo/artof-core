/**
 * @file Line.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Functions for line operations
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Geometry/Point.h>
#include <Utils/Geometry/Transform.h>
#include <math.h>
#include <float.h>
#include <vector>

namespace Ilvo {
namespace Utils {
namespace Geometry {

    enum Side{
        BEGIN,
        END,
        BOTH
    };

    class Line {
        private:
            Point pointStart;
            Point pointStop;
            bool empty;
        public:
            Line();
            Line(Point p1, Point p2);
            ~Line() = default;

            const Point& p1() const;
            const Point& p2() const;

            void p1(Point p1);
            void p2(Point p2);

            bool isEmpty() const;
            double length() const;

            double alpha();
            void params(double &a, double &b, double &c);
            void params(double &m, double &q);
            void extend(double distance, Side side=Side::BOTH);

            Point pointFrom(double distance, Side side=Side::BEGIN);
            Line orthogonal(Point startPoint, double length, bool left=true);
            Point center();
            double distance(Point point);
            bool left(Point c);
            double corner(Line line);
            Point intersection(Line line);
            std::vector<PointPtr> interpolate(double interpolationDistance, bool curvy=false) const;

            Line& operator= (const Line& from);
    };

    inline std::ostream& operator<<(std::ostream& os, const Line& line) {
        os << "x,y,label" << std::endl;
        os << line.p1() << ",p1" << std::endl;
        os << line.p2() << ",p2" << std::endl;
        return os;
    }

} // namespace Ilvo
} // namespace Utils
} // namespace Geometry


