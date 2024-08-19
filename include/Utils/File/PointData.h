/**
 * @file PointData.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Abstraction for all types of shape data (points, polygons, linestrings, ...)
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <vector>
#include <string>
#include <Utils/Geometry/Transform.h>
#include <Utils/Geometry/Point.h>
#include <iomanip>
#include <iostream>


namespace Ilvo {
namespace Utils {
namespace File {

    enum ShapeFieldType { INT, BOOL, STRING, DOUBLE };

    class ShapeFieldData {
        public:
            union {
                int i;
                bool b;
                double d;
                std::string s;
            };
            std::string name;
            ShapeFieldType type;

            ShapeFieldData(std::string name, int v): name(name), i(v) { type = INT; }; 
            ShapeFieldData(std::string name, bool v): name(name), b(v) { type = BOOL; }; 
            ShapeFieldData(std::string name, double v): name(name), d(v) { type = DOUBLE; }; 
            ShapeFieldData(std::string name, std::string v): name(name), s(v) { type = STRING; };
            ~ShapeFieldData() {};
    };

    typedef std::shared_ptr<ShapeFieldData> ShapeFieldDataPtr;

    class PointData
    {
    protected:
        bool polygon;
        std::vector<std::vector<Geometry::PointPtr>> series;
        std::vector<std::vector<ShapeFieldDataPtr>> metadata;
    public:
        PointData(bool polygon);
        ~PointData() = default;

        const std::vector<std::vector<Geometry::PointPtr>>& getAllPoints();
        int getNumSeries();
        bool hasMultiple();

        std::vector<Geometry::PointPtr>& getPoints(uint serie);
        std::vector<ShapeFieldDataPtr>& getFields(uint i);
        int getNumPoints(uint i);
        int getNumFields(uint i);
        bool isPolygon(uint i);
    };

    inline std::ostream & operator<<(std::ostream & str, PointData& data) { 
        str << std::setprecision(15);
        std::vector<std::vector<Geometry::PointPtr>> pv = data.getAllPoints();
        for (int i = 0; i < int(pv.size()); i++) {
            str << "Entity " << i << " has " << data.getNumPoints(i) << " points:" << std::endl;
            for (Geometry::PointPtr p: data.getPoints(i)) {
                str << "(" << p->x() << ", " << p->y() << ") ";
            }
            str << std::endl << "and " << data.getNumFields(i) << " fields:" << std::endl;
            for (ShapeFieldDataPtr f: data.getFields(i)) {
                switch(f->type) {
                    case ShapeFieldType::INT: {
                        str << "(" << f->name << ", " << "int" << ", " << f->i << ")" << std::endl;
                        break;
                    }
                    case ShapeFieldType::DOUBLE: {
                        str << "(" << f->name << ", " << "double" << ", " << f->d << ")" << std::endl;
                        break;
                    }
                    case ShapeFieldType::BOOL: {
                        str << "(" << f->name << ", " << "bool" << ", " << f->b << ")" << std::endl;
                        break;
                    }
                    case ShapeFieldType::STRING: {
                        str << "(" << f->name << ", " << "string" << ", " << f->s << ")" << std::endl;
                        break;
                    }
                }
            }
            str << "---" << std::endl;
        }

        return str;
    }

} // namespace Ilvo
} // namespace Utils
} // namespace File