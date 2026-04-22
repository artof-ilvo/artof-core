#pragma once

#include <string>
#include <gdal/cpl_conv.h>
#include <gdal/gdal_priv.h>
#include <Utils/Geometry/Point.h>
#include <Utils/Geometry/Polygon.h>

namespace Ilvo {
namespace Utils {
namespace Settings {

    class AsAppliedMap
    {
    private:
        std::string& filename;
        GDALDataset  *poDataset;

        double adfGeoTransform[6];      // Pixel    ->  Geo
        double adfInvGeoTransform[6];   // Geo      ->  Pixel
    public:
        AsAppliedMap(std::string& filename);
        ~AsAppliedMap();

        // Prevent copying! 
        AsAppliedMap(const AsAppliedMap&) = delete;
        AsAppliedMap& operator=(const AsAppliedMap&) = delete;

        // Explicitly define or default the move constructor with noexcept
        AsAppliedMap(AsAppliedMap&& other) noexcept; 
        AsAppliedMap& operator=(AsAppliedMap&& other) noexcept;

        void update();
        Geometry::Point geoToPixel(Geometry::Point point);
        Geometry::Point pixelToGeo(Geometry::Point point);

        bool applied(const Geometry::Polygon& p);
    };
    
}
}
}
