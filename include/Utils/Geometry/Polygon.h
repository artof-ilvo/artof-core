/**
 * @file Polygon.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Functions for polygon operations
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <vector>
#include <iomanip>
#include <iostream>
#include <initializer_list>
#include <ThirdParty/json.hpp>
#include <Utils/Geometry/Geometry.h>
#include <Utils/Geometry/Line.h>
#include <Utils/Settings/State.h>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>


namespace Ilvo {
namespace Utils {
namespace Geometry {

    // polygon clockwise and closed
    typedef boost::geometry::model::polygon<bgPoint2D, true, true> bgPolygon2D;

    class Polygon : public Geometry
    {  
    private:
        bgPolygon2D p;
    public:
        Polygon() = default;
        Polygon(std::vector<PointPtr> points);
        Polygon(std::vector<Point> points);
        ~Polygon() = default;
        
        // virtual functions of Geometry
        Point center() const;
        double distance(const Point& p) const;
        void update(Settings::TransformMatrix matrix, double width, double height);
        void update(Settings::TransformMatrix matrix, double width, double up, double down);

        const bgPolygon2D& geometry() const;
        nlohmann::json toJson() const;

        void contour(std::vector<std::vector<double>>& robotLatLng, std::vector<std::vector<double>>& robotXY, int zone=-1) const;
    };

    inline std::ostream & operator<<(std::ostream & Str, Polygon& polygon) { 
        Str << std::setprecision(15);
        Str << "Number of Points: " << polygon.geometry().outer().size() << std::endl;
        for (bgPoint2D p: polygon.geometry().outer()) {
            Str << "point: (" << p.x() << ", " << p.y() << ")" << std::endl;
        }
        Str << "---" << std::endl;

        return Str;
    }

    typedef std::shared_ptr<Polygon> PolygonPtr;
    
} // namespace Ilvo
} // namespace Utils
} // namespace Geometry

