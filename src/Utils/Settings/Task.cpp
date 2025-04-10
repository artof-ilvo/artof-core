#include <Utils/Settings/Task.h>
#include <Utils/Settings/Section.h>
#include <Utils/Logging/LoggerStream.h>
#include <Utils/Geometry/Transform.h>
#include <Exceptions/FileExceptions.hpp>
#include <Exceptions/RobotExceptions.hpp>
#include <Utils/File/File.h>
#include <Utils/File/PointCsvFile.h>
#include <Utils/File/PointShapeFile.h>
#include <Utils/Geometry/Polygon.h>
#include <ThirdParty/bprinter/table_printer.h>

#include <boost/geometry/algorithms/centroid.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/overlaps.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <bits/stdc++.h>
#include <iomanip>      // setprecision

using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::File;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;

using namespace std;
using namespace nlohmann; 
using json = nlohmann::json;
using namespace boost::filesystem;
using namespace boost::geometry;
using bprinter::TablePrinter;
using namespace Eigen;

Task::Task(string baseFilePath, json j_task, int gpsZoneId) :
    platform(Platform::getInstance()),
    gpsZoneId(gpsZoneId),
    hitch(platform.getHitch(j_task["hitch"]))
{
    // initialize component
    if (j_task.contains("name")) name = j_task["name"].get<string>();
    else throw SettingsParamNotFoundException("Task", "name");

    if (j_task.contains("type")) type = j_task["type"].get<string>();
    else throw SettingsParamNotFoundException("Task", "type");

    if (j_task.contains("implement")) {
        implement = Implement(j_task["implement"].get<string>(), platform.robot.width);
    } else {
        implement = Implement(platform.robot.width);
    }

    // read in files
    taskmappath = baseFilePath + "/" + name;
    if ( !exists(taskmappath) ) {
            throw PathNotFoundException(taskmappath);
    }

    vector<string> xFields{"Easting", "X"};
    vector<string> yFields{"Northing", "Y"};
    bool polygon = true;

    string filepath_taskmap = searchFileWithExtension(taskmappath, ".csv");
    if (filepath_taskmap.size() > 0) {
        PointCsvFile f(polygon);
        f.init(filepath_taskmap, xFields, yFields);
        initVariant(f);
    } else {
        filepath_taskmap = searchFileWithExtension(taskmappath, ".shp");
        if (filepath_taskmap.size() > 0) {
            PointShapeFile f(polygon, gpsZoneId);
            f.init(filepath_taskmap);
            initVariant(f);
        } else {
            throw NoShpOrCsvFileException(taskmappath);
        }
    }

}

void Task::initVariant(PointData& f)
{
     if (type.compare("discrete") == 0 || type.compare("intermittent") == 0) { // discrete --> save as points
        geometryType = GeometryType::POINTS;
        PointVector geometries(f.getPoints(0));
        this->points = geometries;
    } else { // continous or hitch task --> save as polygons
        geometryType = GeometryType::POLYGONS;
        vector<PolygonPtr> vec;
        for (int i = 0; i < f.getNumSeries(); i++) {
            vec.push_back(make_shared<Polygon>(f.getPoints(i)));
        }
        PolygonVector geometries(vec);
        this->polygons = geometries;
    }
}

bool Task::compareClosePoints(IndexPointPtr p1, IndexPointPtr p2) 
{
    return p1->index < p2->index;
}

bool Task::equalClosePoints(IndexPointPtr p1, IndexPointPtr p2) 
{
    return p1->index == p2->index;
}

void Task::createPathPointsDiscr(vector<PointPtr> trajectPoints)
{
    for (PointPtr p_task: get<PointVector>(points)) {
        IndexPoint p;
        double d = trajectPoints[0]->distance(*p_task);
        for (int i = 1; i < trajectPoints.size(); i++) {
            double d_to_traj = trajectPoints[i]->distance(*p_task);
            if (d_to_traj < d) {
                p = *trajectPoints[i];
                p.index = i;
                d = d_to_traj;
            }
        }
        discr_path_points.push_back(make_shared<IndexPoint>(p));
    }
    sort(discr_path_points.begin(), discr_path_points.end(), this->compareClosePoints);
    discr_path_points.erase(unique(discr_path_points.begin(), discr_path_points.end(), this->equalClosePoints), discr_path_points.end());
}

void Task::printRapport(LoggerStream& logger) 
{
    stringstream s;
    s << "** Task rapports" << endl;
    if (geometryType == POLYGONS) {
        s << "## Polygon Array ##" << endl;
    
        PolygonVector& polyVec = get<PolygonVector>(polygons);
        for (int i = 0; i < polyVec.size(); i++) {
            s << i << ") " << endl;

            TablePrinter tp(&s);
            tp.AddColumn("X [m]", 15);
            tp.AddColumn("Y [m]", 15);
            tp.PrintHeader();
            for (auto& p: polyVec[i]->geometry().outer()) {
                tp << p.x() << p.y();
            }
            tp.PrintFooter();
        }
        logger << s.str();
    }

    if (geometryType == POINTS) {
        PointVector pointVec = get<PointVector>(points);
        s << "## Point array ##" << endl;
        s << setprecision(15);
        TablePrinter tp(&s);
        if (discr_path_points.size() > 0) {
            tp.AddColumn("idx", 10);
            tp.AddColumn("X_path [m]", 15);
            tp.AddColumn("Y_path [m]", 15);
            tp.PrintHeader();
            for (int i = 0; i < discr_path_points.size(); i++) {
                tp << discr_path_points.at(i)->index << discr_path_points.at(i)->x() << discr_path_points.at(i)->y();
            }
        } else {
            tp.AddColumn("X [m]", 15);
            tp.AddColumn("Y [m]", 15);
            tp.PrintHeader();
            for (int i = 0; i < pointVec.size(); i++) {
                tp << pointVec.at(i)->x() << pointVec.at(i)->y();
            }
        }

        tp.PrintFooter();
    }
}

void Task::updateState(VariableManager* manager)
{
    double actualHitchAngle = manager->getVariable("plc.monitor." + hitch.getEntityName() + ".angle")->getValue<double>();
    
    // calculate new section states
    for (int i = 0; i < implement.getSections().size(); i++) {
        auto section = implement.getSections().at(i);

        double actualParallelAngle = 0.0;
        string sectionFeedbackName = "plc.monitor." + hitch.getEntityName() + ".feedback_sections." + to_string(i);
        if (manager->existsVariable(sectionFeedbackName)) {
            actualParallelAngle = manager->getVariable(sectionFeedbackName)->getValue<double>();
        } else {
            manager->getStream().setRedisValue(sectionFeedbackName, actualParallelAngle);
        }
        section->setParallelAngle(actualParallelAngle);
        section->updateState(hitch.getBallState(actualHitchAngle) * section->getParallelTransform());
        section->updatePolygon();
    }
}


Hitch& Task::getHitch() 
{
    return hitch;
}

const string& Task::getType() 
{
    return type;
}

const string& Task::getName() 
{
    return name;
}

bool Task::isType(string type)
{
    return getType().compare(type) == 0;
}

const GeometryType& Task::getGeometryType()
{
    return geometryType;
}


bool Task::onHitch(string name)
{
    return getHitch().name.compare(name) == 0;
}

const vector<IndexPointPtr>& Task::getPathPointsDiscr()
{
    return discr_path_points;
}

bool Task::updateSections(VariableManager* manager, bool disable)
{
    bool activeSections = false;

    for (int i = 0; i < implement.getSections().size(); i++) {
        auto section = implement.getSections().at(i);
        string name = "plc.control." + hitch.getEntityName() + ".activate_sections." + to_string(i);
        if (getImplement().worksOnTaskmap()) {
            section->setActive(insideTaskMap(section, disable));
            manager->getVariable(name)->setValue<int>((int) section->getActive());
        } else {
            bool active = manager->getVariable(name)->getValue<bool>();
            section->setActive(active);
        }
        if (section->getActive()) {
            activeSections = true;
        }
    }

    return activeSections;
}

bool Task::cardanEnabled(VariableManager* manager, bool disable)
{
    bool activeSections = false;

    for (int i = 0; i < implement.getSections().size(); i++) {
        auto section = implement.getSections().at(i);

        if (insideTaskMap(section, disable)) {
            return true;
        }
    }

    return false;
}

void Task::activateSection(string id, bool value)
{
    for (auto section: implement.getSections()) {
        if (section->id.compare(id) == 0) {
            section->setActive(value);
        }
    }
}

bool Task::insideTaskMap(shared_ptr<Section> section, bool disable)
{
    const Polygon& polygonSection = section->getPolygon();
    Point currentPosition(section->getState().getT().asVector());
    section->clearActivationGeometry();

    if (type.compare("continuous") == 0) { 
        for (PolygonPtr polygon: get<PolygonVector>(polygons)) { 
            if (overlaps(polygonSection.geometry(), polygon->geometry()) || covered_by(polygonSection.geometry(), polygon->geometry())) {
                section->setActivationGeometry(polygon);
                return !disable;
            }
        }     
    } else if (type.compare("cardan") == 0) {
        for (PolygonPtr polygon: get<PolygonVector>(polygons)) { 
            if (overlaps(polygonSection.geometry(), polygon->geometry()) || covered_by(polygonSection.geometry(), polygon->geometry())) {
                // section->setActivationGeometry(polygon);
                return !disable;
            }
        } 
    } else if (type.compare("intermittent") == 0) {
        PointVector vec = get<PointVector>(points);
        std::vector<PointPtr> points = vec.nearby(currentPosition, 40);
        for (PointPtr point: points) {
            if (covered_by(point->geometry(), polygonSection.geometry())) {
                section->addActivationGeometry(point);
                return !disable;
            }
        }   
    }

    return false;
}

bool Task::insideTaskMap(Point point, bool disable)
{
    PolygonVector& vec = get<PolygonVector>(polygons);
    if (vec.size() > 0) {
        for (PolygonPtr polygon: vec) { 
            if (covered_by(point.geometry(), polygon->geometry())) {
                return !disable;
            }
        }
        return false;
    }

    return false;
}

bool Task::hitchInTaskMap()
{
    return insideTaskMap(hitch.getState().getT().asVector());
}

Implement& Task::getImplement()
{
    return implement;
}

json Task::toJson() const
{
    json j;

    json j_points = json::array();

    if (geometryType == POLYGONS) {
        PolygonVector polyVec = get<PolygonVector>(polygons);
        for (PolygonPtr polygonPtr: polyVec) {
            j_points.push_back(polygonPtr->toJson());
        }
    } else {
        PointVector pointVec = get<PointVector>(points);
        j_points.push_back(json::array());  // same look as polygon array
        for (PointPtr pointPtr: pointVec) {
            j_points[0].push_back(pointPtr->toJson());
        }
    }


    j["name"] = this->name;
    j["hitch"] = this->hitch.name;
    j["type"] = this->type;
    j["points"] = j_points;

    return j;
}