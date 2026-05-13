#include <Utils/File/PointGeoJsonFile.h>
#include <Utils/Geometry/Transform.h>
#include <Utils/Logging/LoggerStream.h>
#include <Exceptions/FileExceptions.hpp>
#include <fstream>
#include <filesystem>

using namespace Ilvo::Utils::File;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;
using namespace nlohmann;
using namespace std;

PointGeoJsonFile::PointGeoJsonFile(int utmZone)
    : PointData(false), utmZone(utmZone)
{
}

void PointGeoJsonFile::init(const std::string& filename)
{
    if (!filesystem::exists(filename))
        throw PathNotFoundException(filename);

    ifstream f(filename);
    json j = json::parse(f);

    series.clear();
    metadata.clear();

    // Lees features met type "swath" of "headland" uit data.geojson
    for (auto& feature : j["features"]) {
        if (!feature["properties"]["type"].is_string()) continue;
        string type = feature["properties"]["type"].get<string>();
        if (type == "swath" || type == "headland")
            addSegment(feature);
    }
}

void PointGeoJsonFile::addSegment(const json& feature)
{
    auto& props = feature["properties"];
    auto& coords = feature["geometry"]["coordinates"];

    vector<PointPtr> points;
    for (auto& coord : coords) {
        // GeoJSON standaard: [lng, lat]
        double lng = coord[0].get<double>();
        double lat = coord[1].get<double>();
        double x, y;
        LatLonToUTMXY(lat, lng, utmZone, x, y);
        points.push_back(make_shared<Point>(x, y));
    }

    vector<ShapeFieldDataPtr> fields;
    fields.push_back(make_shared<ShapeFieldData>("algorithm", props["algorithm"].get<string>()));
    fields.push_back(make_shared<ShapeFieldData>("sensor",    props["sensor"].get<string>()));

    if (props.contains("fallback_algorithm")) {
        fields.push_back(make_shared<ShapeFieldData>("fallback_algorithm", props["fallback_algorithm"].get<string>()));
        fields.push_back(make_shared<ShapeFieldData>("fallback_sensor",    props["fallback_sensor"].get<string>()));
    }

    series.push_back(points);
    metadata.push_back(fields);
}
