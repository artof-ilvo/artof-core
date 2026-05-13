#include <Utils/Settings/Field.h>
#include <Utils/File/File.h>
#include <Utils/File/PointCsvFile.h>
#include <Utils/File/PointShapeFile.h>
#include <Utils/File/PointGeoJsonFile.h>
#include <Utils/Geometry/Transform.h>
#include <Utils/Logging/LoggerStream.h>
#include <Exceptions/FileExceptions.hpp>

#include <boost/filesystem.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>

#include <fstream>
#include <iostream>
#include <algorithm>

using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::File;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;

using namespace std;
using namespace boost::filesystem;
using namespace nlohmann; 
using json = nlohmann::json;

Field::Field()
{}

string Field::checkFieldName(string name) {
    string dirPath = string(getenv("ILVO_PATH")) + "/field";
    string fieldPath = dirPath + "/" + name;

    if ( !exists(fieldPath)) {
        throw PathNotFoundException(fieldPath);
    }

    return name;
}

Field::Field(std::string name, int zoneId) : 
    baseFilePath(string(getenv("ILVO_PATH"))),
    name(name),
    trajectFilePath(string(getenv("ILVO_PATH")) + "/field/" + name + "/traject"),
    geofenceFilePath(string(getenv("ILVO_PATH")) + "/field/" + name + "/geofence"),
    infoFilePath(string(getenv("ILVO_PATH")) + "/field/" + name + "/info.json"),
    gpsZoneId(zoneId)
{
    string fieldPath = baseFilePath + "/field/" + name;
    if ( !exists(fieldPath)) {
        throw PathNotFoundException(fieldPath);
    }

    string dataGeoJsonPath = baseFilePath + "/field/" + name + "/data.geojson";

    LoggerStream::getInstance() << DEBUG << "Loading field: " << name;
    LoggerStream::getInstance() << DEBUG << "baseFilePath: " << baseFilePath;
    LoggerStream::getInstance() << DEBUG << "trajectFilePath: " << trajectFilePath;
    LoggerStream::getInstance() << DEBUG << "geofenceFilePath: " << geofenceFilePath;
    LoggerStream::getInstance() << DEBUG << "infoFilePath: " << infoFilePath;

    bool hasDataGeoJson = exists(dataGeoJsonPath);

    if (!hasDataGeoJson) {
        if ( !exists(trajectFilePath) )
            throw PathNotFoundException(trajectFilePath);
        if ( !exists(geofenceFilePath) )
            throw PathNotFoundException(geofenceFilePath);
    }

    if ( exists(infoFilePath) ) {
        std::fstream f(infoFilePath);
        try {
            fieldInfo = json::parse(f);
        } catch(json::exception& e) {
            LoggerStream::getInstance() << ERROR << "Field info file parse error, file name: \"" << name << "\", file path: \"" << infoFilePath << "\", " << e.what();
            throw runtime_error("Field info file parse error, " + std::string(e.what()));
        }
    }

    // parse the json data
    if (fieldInfo.contains("name") && fieldInfo["name"].is_string()) {
        this->name = fieldInfo["name"].get<string>();
        removeCharacters(this->name, CHARS_BRACKETS);
    }

    // read in files
    vector<string> xFields{"Easting", "X"};
    vector<string> yFields{"Northing", "Y"};
    bool polygon = false;

    if (hasDataGeoJson) {
        // Lees alles uit data.geojson
        ifstream geoFile(dataGeoJsonPath);
        json geoJson = json::parse(geoFile);

        bool hasTraject = false;
        bool hasGeofence = false;
        PointGeoJsonFile btSegments(gpsZoneId);
        btSegments.init(dataGeoJsonPath);

        for (auto& feature : geoJson["features"]) {
            string featType = feature["properties"]["type"].is_string() ? feature["properties"]["type"].get<string>() : "";
            string featName = feature["properties"]["name"].is_string() ? feature["properties"]["name"].get<string>() : "";

            if (featName == "traject" && feature["geometry"]["type"] == "LineString") {
                // Traject punten
                for (auto& coord : feature["geometry"]["coordinates"]) {
                    double lng = coord[0].get<double>();
                    double lat = coord[1].get<double>();
                    double x, y;
                    LatLonToUTMXY(lat, lng, gpsZoneId, x, y);
                    trajectPoints.push_back(make_shared<Point>(x, y));
                }
                hasTraject = true;

            } else if (featName == "geofence" || featType == "polygon") {
                // Geofence
                vector<PointPtr> pts;
                for (auto& coord : feature["geometry"]["coordinates"][0]) {
                    double lng = coord[0].get<double>();
                    double lat = coord[1].get<double>();
                    double x, y;
                    LatLonToUTMXY(lat, lng, gpsZoneId, x, y);
                    pts.push_back(make_shared<Point>(x, y));
                }
                if (featName == "geofence") {
                    geofence = Polygon(pts);
                    hasGeofence = true;
                }

            } else if (featType == "task") {
                // Task vanuit GeoJSON feature
                string taskName = featName;
                json taskInfo;
                if (fieldInfo.contains("tasks") && fieldInfo["tasks"].is_array()) {
                    for (auto& t : fieldInfo["tasks"]) {
                        if (t.contains("name") && t["name"].is_string() && t["name"].get<string>() == taskName) {
                            taskInfo = t;
                            break;
                        }
                    }
                }
                if (!taskInfo.is_null())
                    tasks.push_back(Task(feature, taskInfo, gpsZoneId));
            }
        }

        // BT segmenten (swaths/headlands) uit dezelfde file
        if (btSegments.getNumSeries() > 0)
            this->trajectSegments = btSegments;

        // Fallback naar shapefiles als geojson onvolledig is
        if (!hasTraject) {
            string _trajectFilePath = searchFileWithExtension(trajectFilePath, ".shp");
            if (_trajectFilePath.size() > 0) {
                PointShapeFile f(false, gpsZoneId);
                f.init(_trajectFilePath);
                this->trajectPoints = f.getPoints(0);
            }
        }
        if (!hasGeofence) {
            string _geofenceFilePath = searchFileWithExtension(geofenceFilePath, ".shp");
            if (_geofenceFilePath.size() > 0) {
                PointShapeFile f(true, gpsZoneId);
                f.init(_geofenceFilePath);
                geofence = Polygon(f.getPoints(0));
            }
        }
        if (tasks.empty() && fieldInfo.contains("tasks") && fieldInfo["tasks"].is_array()) {
            for (json task : fieldInfo["tasks"]) {
                string taskDirectoryPath = baseFilePath + "/field/" + name + "/tasks";
                tasks.push_back(Task(taskDirectoryPath, task, gpsZoneId));
            }
        }

    } else {
        // shapefiles
        if (fieldInfo.contains("tasks") && fieldInfo["tasks"].is_array()) {
            for (json task : fieldInfo["tasks"]) {
                string taskDirectoryPath = baseFilePath + "/field/" + name + "/tasks";
                tasks.push_back(Task(taskDirectoryPath, task, gpsZoneId));
            }
        }

        string _trajectFilePath = searchFileWithExtension(trajectFilePath, ".csv");
        if (_trajectFilePath.size() > 0) {
            PointCsvFile f(false);
            f.init(_trajectFilePath, xFields, yFields);
            this->trajectPoints = f.getPoints(0);
        } else {
            _trajectFilePath = searchFileWithExtension(trajectFilePath, ".shp");
            if (_trajectFilePath.size() > 0) {
                PointShapeFile f(false, gpsZoneId);
                f.init(_trajectFilePath);
                this->trajectPoints = f.getPoints(0);
            } else {
                throw NoShpOrCsvFileException(trajectFilePath);
            }
        }

        polygon = true;
        string _geofenceFilePath = searchFileWithExtension(geofenceFilePath, ".csv");
        if (_geofenceFilePath.size() > 0) {
            PointCsvFile f(true);
            f.init(_geofenceFilePath, xFields, yFields);
            geofence = Polygon(f.getPoints(0));
        } else {
            _geofenceFilePath = searchFileWithExtension(geofenceFilePath, ".shp");
            if (_geofenceFilePath.size() > 0) {
                PointShapeFile f(true, gpsZoneId);
                f.init(_geofenceFilePath);
                geofence = Polygon(f.getPoints(0));
            } else {
                throw NoShpOrCsvFileException(geofenceFilePath);
            }
        }
    }
}

// Field& Field::operator=(const Field& other)
// {
//     trajectFilePath = other.trajectFilePath;
//     geofenceFilePath = other.geofenceFilePath;
//     infoFilePath = other.infoFilePath;

//     baseFilePath = other.baseFilePath;
//     name = other.name;
//     gpsZoneId = other.gpsZoneId;

//     fieldInfo = other.fieldInfo;
//     geofence = other.geofence;

//     tasks.clear();
//     copy(other.tasks.begin(), other.tasks.end(), back_inserter(tasks));

//     trajectPoints.clear();
//     trajectPoints.insert(trajectPoints.begin(), other.trajectPoints.begin(), other.trajectPoints.end());

//     return *this;
// }

const std::vector<PointPtr>& Field::getTrajectPoints() const
{
    return trajectPoints;
}

bool Field::hasTrajectSegments() const
{
    return trajectSegments.has_value();
}

PointGeoJsonFile& Field::getTrajectSegments()
{
    return trajectSegments.value();
}


const Polygon& Field::getGeofence() const
{
    return geofence;
}

vector<Task>& Field::getTasks()
{
    return tasks;
}

bool Field::hasTasksWithType(string type)
{
    for (Task& task: tasks) {
        if (task.isType(type)) {
            return true;
        }
    }
    return false;
}

bool Field::hasTaskWithTypeOnHitch(string type, string hitchName) {
    for (Task& task: tasks) {
        if (task.isType(type) && task.getHitch().name == hitchName) {
            return true;
        }
    }
    return false;
}

Task& Field::getTaskWithType(string type)
{
    for (Task& task: tasks) {
        if (task.isType(type)) {
            return task;
        }
    }
    throw NoTasksWithTypeFound(type);
}

Task& Field::getTaskWithTypeOnHitch(string type, string hitchName)
{
    for (Task& task: tasks) {
        if (task.isType(type)) {
            return task;
        }
    }
    throw NoTasksWithTypeFound(type);
}

nlohmann::json Field::toJson() const
{
    json j;

    json j_tasks = json::array();
    for (const Task& task: tasks) {
        j_tasks.push_back(task.toJson());
    }
    json j_trajectPoints = json::array();
    for (auto pointPtr: this->trajectPoints) {
        j_trajectPoints.push_back(pointPtr->toJson());
    }

    j["name"] = this->name;
    j["traject_points"] = j_trajectPoints;
    j["geofence"] = geofence.toJson();
    j["tasks"] = j_tasks;

    return j;
}

void Field::writeJson(const std::string& relativePath) const
{
    std::ofstream jsonfile;
    string file_path = baseFilePath + "/" + relativePath;
    jsonfile.open(file_path);
    if (!jsonfile.is_open())
    {
        throw PathNotFoundException(file_path);
    }
    jsonfile << this->toJson();
    jsonfile.close();
}

void Field::writeJson() const
{
    std::ofstream jsonfile;
    string file_path = baseFilePath + "/field/" + name + "/field.json";
    jsonfile.open(file_path);
    if (!jsonfile.is_open())
    {
        throw PathNotFoundException(file_path);
    }
    jsonfile << this->toJson();
    jsonfile.close();
}