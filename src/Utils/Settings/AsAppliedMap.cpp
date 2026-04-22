#include <Utils/Settings/AsAppliedMap.h>
#include <Exceptions/RasterExceptions.hpp>
#include <boost/filesystem.hpp>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Exception;
using namespace boost::filesystem;

AsAppliedMap::AsAppliedMap(std::string& filename) : filename(filename)
{
    // 1. Register GDAL 
    GDALAllRegister();
    // 2. Open the file to extract the geo transformations.
    update();
}

AsAppliedMap::~AsAppliedMap()
{
    if (poDataset) {
        GDALClose((GDALDatasetH)poDataset);
        poDataset = nullptr; // Set to null after closing
    }
}

Point AsAppliedMap::geoToPixel(Point point)
{
    double xGeo = adfGeoTransform[0] + point.x() * adfGeoTransform[1] + point.y() * adfGeoTransform[2];   
    double yGeo = adfGeoTransform[3] + point.x() * adfGeoTransform[4] + point.y() * adfGeoTransform[5];
    return Point(xGeo, yGeo);
}

Point AsAppliedMap::pixelToGeo(Point point)
{
    int xPix = adfInvGeoTransform[0] + point.x() * adfInvGeoTransform[1] + point.y() * adfInvGeoTransform[2];   
    int yPix = adfInvGeoTransform[3] + point.x() * adfInvGeoTransform[4] + point.y() * adfInvGeoTransform[5];
    return Point(xPix, yPix);
}

void AsAppliedMap::update()
{
    bool updateTransforms = (poDataset == NULL); // Only update when the poDataset was not registered yet

    if (exists(filename)) {
        poDataset = (GDALDataset *) GDALOpen(filename.c_str(), GA_ReadOnly);
        if (poDataset != NULL) {
            if (updateTransforms) {
                if (poDataset->GetGeoTransform(adfGeoTransform)) /* extracting transformation ok */ ;
                if (GDALInvGeoTransform(adfGeoTransform, adfInvGeoTransform)) /* inverting transformation ok */ ;
            }
        }
        GDALClose(poDataset);
    }
}

bool AsAppliedMap::applied(const Polygon& polygon)
{
    // If no raster file is loaded return false
    if(poDataset == NULL) {
        return false;
    }

    // Process further if the raster file is loaded
    Point minGeo, maxGeo;
    polygon.envelope(minGeo, maxGeo);

    Point p1 = geoToPixel(minGeo);
    Point p2 = geoToPixel(maxGeo);

    // 1. Calculate the bounding box in pixel space
    // We use min/max to ensure coordinates are valid even if the geotransform flips axes
    int nXOff = std::max(0, (int)std::min(p1.x(), p2.x()));
    int nYOff = std::max(0, (int)std::min(p1.y(), p2.y()));
    int nXSize = std::min((int)std::abs(p1.x() - p2.x()), poDataset->GetRasterXSize() - nXOff);
    int nYSize = std::min((int)std::abs(p1.y() - p2.y()), poDataset->GetRasterYSize() - nYOff);

    // Safety check for empty intersection
    if (nXSize <= 0 || nYSize <= 0) return false;

    // 2. Allocate a buffer for the chunk of data
    // Assuming the data is float; change to int or double if necessary
    std::vector<float> scanline(nXSize * nYSize);

    // 3. Read the data from the first band (index 1)
    GDALRasterBand* poBand = poDataset->GetRasterBand(1);
    CPLErr err = poBand->RasterIO(GF_Read, nXOff, nYOff, nXSize, nYSize, 
                                  scanline.data(), nXSize, nYSize, GDT_Float32, 0, 0);

    if (err != CE_None) return false;

    // 4. Iterate through the buffer to check values
    for (float val : scanline) {
        if (val > 0) return true; 
    }
    
    return false;
}