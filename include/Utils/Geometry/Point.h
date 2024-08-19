/**
 * @file Point.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Functions for point operations
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
#include <ThirdParty/json.hpp>
#include <ThirdParty/Eigen/Geometry> 
#include <Utils/Redis/Variable.h>
#include <Utils/Geometry/Geometry.h>
#include <Utils/Logging/LoggerStream.h>
#include <boost/geometry/geometries/point_xy.hpp>

namespace Ilvo {
namespace Utils {
namespace Geometry {

    class Point : public Geometry {
        protected:
            bgPoint2D p;
            bool empty;
        public:           
            Point();
            Point(double x, double y);
            Point(std::shared_ptr<Point> pointPtr);
            Point(Eigen::Vector3d t);
            Point(Eigen::Affine3d s);

            Point& operator= (const Eigen::Vector3d& v);
            Point& operator= (const Eigen::Affine3d& m);
            Point& operator= (const Point& from);

            operator Eigen::Vector3d();

            const double x() const;
            const double y() const;
            bool isEmpty() const;

            void x(double x);
            void y(double y);

            const bgPoint2D& geometry() const;

            // virtual functions of Geometry
            Point center() const;
            double distance(const Point& p) const;

            nlohmann::json toJson() const;

    };

    // register inheritted class for boost algorithms
    // BOOST_GEOMETRY_REGISTER_POINT_2D(Point, double, boost::geometry::cs::cartesian, x(), y())

    inline bool operator==(const Point& p1, const Point& p2) {
        return (p1.x() == p2.x() && p1.y() == p2.y());
    }

    inline std::ostream& operator<<(std::ostream& os, const Point& p) {
        os << std::setprecision(10) << p.x() << ", " << p.y();
        return os;
    }
    inline Utils::Logging::LoggerStream& operator<<(Utils::Logging::LoggerStream& os, const Point& p) {
        os << std::setprecision(13) << p.x() << ", " << p.y();
        return os;
    }

    typedef std::shared_ptr<Point> PointPtr;

    class CornerPoint
    {
    public:
        CornerPoint();
        CornerPoint(const CornerPoint& from);
        CornerPoint(int index, Point point, double angle, int cornerIndex, Point previousRawPoint=Point(), Point nextRawPoint=Point());
        ~CornerPoint() = default;

        CornerPoint& operator= (const CornerPoint& from);

        /** index in the interpolated path */
        int index;
        /** point in coordinates */
        Point point;
        /** angle in degrees */
        double angle;
        /** index of the corner */
        int cornerIndex;
        /** old direction of the path */
        Point previousRawPoint;
        /** new direction of the path */
        Point nextRawPoint;
        /** is part of the headland */
        bool isHeadland;
        /** distance of the headland */
        double headlandDistance;

        void setHeadland(double distance);
    };

    inline std::ostream& operator<<(std::ostream& os, const CornerPoint& c) {
        os << c.cornerIndex << "," << c.point << "," << c.angle << "," <<  c.index << "," << (c.isHeadland ? 1: 0);
        return os;
    }
    inline Utils::Logging::LoggerStream& operator<<(Utils::Logging::LoggerStream& os, const CornerPoint& c) {
        os << "Corner: " << c.cornerIndex << " at point (" << c.point << ") of angle: " << c.angle << "° " << "on path index " << c.index << " " << (c.isHeadland ? "(HEADLAND)" : "");
        return os;
    }

    typedef std::shared_ptr<CornerPoint> CornerPointPtr;

    class CurvyPoint: public Point
    {
    public:
        CurvyPoint();
        CurvyPoint(const CurvyPoint& from);
        CurvyPoint(const Point& point);
        CurvyPoint(const Point& point, double radius);
        CurvyPoint(double x, double y);
        CurvyPoint(double x, double y, double radius);
        ~CurvyPoint() = default;

        double radius;
    };

    typedef std::shared_ptr<CurvyPoint> CurvyPointPtr;


    class NextAndPreviousCorner
    {
    public:
        NextAndPreviousCorner() = default;
        NextAndPreviousCorner(CornerPoint previousCorner, CornerPoint nextCorner);
        ~NextAndPreviousCorner() = default;

        CornerPoint previousCorner;
        CornerPoint nextCorner;
    };

    class IndexPoint : public Point
    {
    public:
        IndexPoint();
        IndexPoint(Point point, int index);
        IndexPoint(double x, double y, int index);
        ~IndexPoint() = default;

        IndexPoint& operator= (const IndexPoint& from);
        IndexPoint& operator= (const Point& from);

        int index;
    };

    typedef std::shared_ptr<IndexPoint> IndexPointPtr;

} // namespace Ilvo
} // namespace Utils
} // namespace Geometry

