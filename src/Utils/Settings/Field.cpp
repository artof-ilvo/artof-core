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

    LoggerStream::getInstance() << DEBUG << "Loading field: " << name;
    LoggerStream::getInstance() << DEBUG << "baseFilePath: " << baseFilePath;
    LoggerStream::getInstance() << DEBUG << "trajectFilePath: " << trajectFilePath;
    LoggerStream::getInstance() << DEBUG << "geofenceFilePath: " << geofenceFilePath;
    LoggerStream::getInstance() << DEBUG << "infoFilePath: " << infoFilePath;

    // Load info.json if present (optional for GeoJSON fields)
    if ( exists(infoFilePath) ) {
        std::fstream f(infoFilePath);
        try {
            fieldInfo = json::parse(f);
        } catch(json::exception& e) {
            LoggerStream::getInstance() << ERROR << "Field info file parse error, file name: \"" << name << "\", file path: \"" << infoFilePath << "\", " << e.what();
            throw runtime_error("Field info file parse error, " + std::string(e.what()));
        }
        if (fieldInfo.contains("name") && fieldInfo["name"].is_string()) {
            this->name = fieldInfo["name"].get<string>();
            removeCharacters(this->name, CHARS_BRACKETS);
        }
    }

    string dataGeoJsonPath = baseFilePath + "/field/" + name + "/data.geojson";
    bool hasDataGeoJson = exists(dataGeoJsonPath);

    vector<string> xFields{"Easting", "X"};
    vector<string> yFields{"Northing", "Y"};

    if (hasDataGeoJson) {
        // --- GeoJSON path ---
        ifstream geoFile(dataGeoJsonPath);
        json geoJson = json::parse(geoFile);

        bool hasTraject  = false;
        bool hasGeofence = false;

        // Parse BT segments (swath/headland with algorithm/sensor metadata)
        PointGeoJsonFile btSegments(gpsZoneId);
        btSegments.init(dataGeoJsonPath);
        if (btSegments.getNumSeries() > 0)
            this->trajectSegments = btSegments;

        for (auto& feature : geoJson["features"]) {
            auto& props = feature["properties"];
            string featType = props["type"].is_string() ? props["type"].get<string>() : "";
            string featName = props["name"].is_string() ? props["name"].get<string>() : "";

            if (featName == "traject" && feature["geometry"]["type"] == "LineString") {
                for (auto& coord : feature["geometry"]["coordinates"]) {
                    double lng = coord[0].get<double>();
                    double lat = coord[1].get<double>();
                    double x, y;
                    LatLonToUTMXY(lat, lng, gpsZoneId, x, y);
                    trajectPoints.push_back(make_shared<Point>(x, y));
                }
                hasTraject = true;

            } else if (featName == "geofence" || featType == "polygon") {
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
                // Build taskInfo from GeoJSON properties
                if (props.contains("hitch_type") && props.contains("hitch_name")
                    && !props["hitch_type"].is_null() && !props["hitch_name"].is_null()) {
                    json taskInfo;
                    taskInfo["name"]  = featName;
                    taskInfo["type"]  = props["hitch_type"];
                    taskInfo["hitch"] = props["hitch_name"];
                    if (props.contains("implement") && !props["implement"].is_null())
                        taskInfo["implement"] = props["implement"];
                    tasks.push_back(Task(feature, taskInfo, gpsZoneId));
                }
            }
        }

        // Fallback to shp if traject/geofence not in GeoJSON
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


    } else {
        // --- Original shp/info.json path ---
        if ( !exists(trajectFilePath) )
            throw PathNotFoundException(trajectFilePath);
        if ( !exists(geofenceFilePath) )
            throw PathNotFoundException(geofenceFilePath);

        // Tasks from info.json
        if (fieldInfo.contains("tasks") && fieldInfo["tasks"].is_array()) {
            for (json task : fieldInfo["tasks"]) {
                string taskDirectoryPath = baseFilePath + "/field/" + name + "/tasks";
                tasks.push_back(Task(taskDirectoryPath, task, gpsZoneId));
            }
        }

        // Traject
        bool polygon = false;
        string _trajectFilePath = searchFileWithExtension(trajectFilePath, ".csv");
        if (_trajectFilePath.size() > 0) {
            PointCsvFile f(polygon);
            f.init(_trajectFilePath, xFields, yFields);
            this->trajectPoints = f.getPoints(0);
        } else {
            _trajectFilePath = searchFileWithExtension(trajectFilePath, ".shp");
            if (_trajectFilePath.size() > 0) {
                PointShapeFile f(polygon, gpsZoneId);
                f.init(_trajectFilePath);
                this->trajectPoints = f.getPoints(0);
            } else {
                throw NoShpOrCsvFileException(trajectFilePath);
            }
        }

        // Geofence
        polygon = true;
        string _geofenceFilePath = searchFileWithExtension(geofenceFilePath, ".csv");
        if (_geofenceFilePath.size() > 0) {
            PointCsvFile f(polygon);
            f.init(_geofenceFilePath, xFields, yFields);
            geofence = Polygon(f.getPoints(0));
        } else {
            _geofenceFilePath = searchFileWithExtension(geofenceFilePath, ".shp");
            if (_geofenceFilePath.size() > 0) {
                PointShapeFile f(polygon, gpsZoneId);
                f.init(_geofenceFilePath);
                geofence = Polygon(f.getPoints(0));
            } else {
                throw NoShpOrCsvFileException(geofenceFilePath);
            }
        }
    }
}


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
        throw PathNotFoundException(file_path);
    jsonfile << this->toJson();
    jsonfile.close();
}

void Field::writeJson() const
{
    std::ofstream jsonfile;
    string file_path = baseFilePath + "/field/" + name + "/field.json";
    jsonfile.open(file_path);
    if (!jsonfile.is_open())
        throw PathNotFoundException(file_path);
    jsonfile << this->toJson();
    jsonfile.close();
}
