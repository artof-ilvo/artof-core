/**
 * @file PointShapeFile.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Extract PointData from shapefile
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <Utils/Geometry/Point.h>
#include <Utils/File/PointData.h>

#include <ThirdParty/shapefil.h>

#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

namespace Ilvo {
namespace Utils {
namespace File {

    /**
     * @brief Loads a shapefile and converts it to PointData.
     * 
     */
    class PointShapeFile : public PointData
    {
    private:
        std::string shp_file;
        std::string dbf_file;

        int utmZone;

        void readShapefile();
        void loadDbfFields(DBFHandle handle, int i);
    public:
        void init(const std::string& filename);

        PointShapeFile(bool polygon, int utmZone);
        ~PointShapeFile() = default;
    };

} // namespace Ilvo
} // namespace Utils
} // namespace File


