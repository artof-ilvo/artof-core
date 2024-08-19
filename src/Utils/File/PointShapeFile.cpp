#include <Utils/File/PointShapeFile.h>
#include <Utils/Logging/LoggerStream.h>
#include <Exceptions/FileExceptions.hpp>
#include <filesystem>

using namespace Ilvo::Utils::File;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;

using namespace std::filesystem;
using namespace std;

PointShapeFile::PointShapeFile(bool polygon, int utmZone) : 
    PointData(polygon), utmZone(utmZone)
{}

void PointShapeFile::init(const std::string& filename)
{
    shp_file = filename;
    // Check if filename is of .shp format
    std::string delimiter = ".";

    // Check file extension
    size_t pos = 0;
    std::string token;
    pos = filename.find(delimiter);
    std::string extention = filename.substr(pos, filename.length());

    if (extention != ".shp")
    {
        LoggerStream::getInstance() << WARN << "No .shp file provided!";
        throw WrongFileExtensionException(extention, ".shp");
    }

    // Create dbf filename
    dbf_file = filename.substr(0, pos);
    dbf_file.append(".dbf");

    // Check if .dbf and .shp file exists
    if ( !exists( shp_file ) )
    {
        throw PathNotFoundException(shp_file);
    }
    if ( !exists( dbf_file ) )
    {
        throw PathNotFoundException(dbf_file);
    }

    // clear array
    series.clear();
    metadata.clear();

    // Read series
    this->readShapefile();
}

void PointShapeFile::loadDbfFields(DBFHandle dbfHandle, int i)
{
    // read in the attributes      
    for (int f = 0; f < DBFGetFieldCount(dbfHandle); f++)
    {
        char fieldNameChar[12];
        DBFFieldType fieldType = DBFGetFieldInfo(dbfHandle, f, fieldNameChar, NULL, NULL);
        string fieldName = string(fieldNameChar);
        switch(fieldType) {
            case FTString:
            {
                this->metadata[i].push_back(make_shared<ShapeFieldData>(fieldName, DBFReadStringAttribute(dbfHandle, i, f)));
                break; 
            }
            case FTInteger:
            {
                this->metadata[i].push_back(make_shared<ShapeFieldData>(fieldName, DBFReadIntegerAttribute(dbfHandle, i, f)));
                break; 
            }
            case FTDouble:
            {
                this->metadata[i].push_back(make_shared<ShapeFieldData>(fieldName, DBFReadDoubleAttribute(dbfHandle, i, f)));
                break; 
            }
            case FTLogical:
            {
                this->metadata[i].push_back(make_shared<ShapeFieldData>(fieldName, DBFReadLogicalAttribute(dbfHandle, i, f)));
                break; 
            }
        }
    }

}


void PointShapeFile::readShapefile()
{
    try
    {
        SHPHandle shpHandle = SHPOpen(shp_file.c_str(), "rb");
        DBFHandle dbfHandle = DBFOpen(shp_file.c_str(), "rb");
        // if (shpHandle <= 0)
        // {
        //     throw std::string("File " + filename + " not found");
        // }

        int nShapeType, nEntities;
        double adfMinBound[4], adfMaxBound[4];
        SHPGetInfo(shpHandle, &nEntities, &nShapeType, adfMinBound, adfMaxBound );

        for (int i = 0; i < nEntities; i++)
        {
            SHPObject* psShape = SHPReadObject(shpHandle, i );

            // For Point and MultiPoint files
            if (psShape->nSHPType == SHPT_POINTZ || psShape->nSHPType == SHPT_POINT
                || psShape->nSHPType == SHPT_MULTIPOINT || psShape->nSHPType == SHPT_MULTIPOINTZ)
            {
                // only push one point vector
                if (i == 0) {
                    this->series.push_back(vector<PointPtr>());
                    this->metadata.push_back(vector<ShapeFieldDataPtr>());
                }

                double x = (psShape->padfX)[0];
                double y = (psShape->padfY)[0];
                // if x y values are in xE[-180,+180] and yE[-90,+90]
                // x => longitude and y => latitude
                // so convert to UTM
                if (x <= 180.0 || y <= 90.0) {
                    double lon = x;
                    double lat = y;
                    LatLonToUTMXY(lat, lon, utmZone, x, y);
                }
                this->series[0].push_back(make_shared<Point>(x, y));
                // if polygon push last point to the array
                if (polygon) {
                    if (i == nEntities-1) {
                        vector<PointPtr> p_first_series = this->getPoints(0);
                        this->series[0].push_back(make_shared<Point>(p_first_series[0]->x(), p_first_series[0]->y()));
                    }
                }

                loadDbfFields(dbfHandle, 0);
            } // For Polygon and linestring files
            else if (
                ((psShape->nSHPType == SHPT_POLYGON || psShape->nSHPType == SHPT_POLYGONZ))
                || (psShape->nSHPType == SHPT_ARC || psShape->nSHPType == SHPT_ARCZ)
            )
            {
                if (psShape->nParts != 1) {
                    // only polygons in once piece (whitout holes)
                    LoggerStream::getInstance() << ERROR << "A polygon with zero or more then one parts detected! Polygons with holes are not permitted!";
                    throw PolygonWithHoleException(); // TODO problems with string conversion
                }
                // only push one point vector
                this->series.push_back(vector<PointPtr>()); // add new pointvector
                this->metadata.push_back(vector<ShapeFieldDataPtr>());

                double* x = psShape->padfX;
                double* y = psShape->padfY;
                
                // read in the shape vertices
                for (int v = 0; v < psShape->nVertices; v++)
                {
                    double x = (psShape->padfX)[v];
                    double y = (psShape->padfY)[v];
                    // if x y values are in xE[-180,+180] and yE[-90,+90]
                    // x => longitude and y => latitude
                    // so convert to UTM
                    if (x <= 180.0 || y <= 90.0) {
                        double lon = x;
                        double lat = y;
                        LatLonToUTMXY(lat, lon, utmZone, x, y);
                    }
                    this->series[i].push_back(make_shared<Point>(x, y));
                }

                loadDbfFields(dbfHandle, i);
            } 

            SHPDestroyObject(psShape);
        }
        SHPClose(shpHandle);
        DBFClose(dbfHandle);
    }
    catch(const std::string& s)
    {
        throw s;
    }
    catch(...)
    {
        throw std::string("Other exception");
    }
}
