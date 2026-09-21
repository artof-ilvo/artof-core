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
        vector<TaskPointPtr> vec;

        vector<Geometry::PointPtr> flatPoints = f.getAllPointsFlat();
        vector<int> routineValues;
        try {
            routineValues = f.getAllFieldsByName<int>("routine");
        } catch (std::runtime_error& e) {
            LoggerStream::getInstance() << WARN << "No routine field of type int found in task " << name << ", defaulting to 0";
            LoggerStream::getInstance() << WARN << "Error: " << e.what();
        }

        if (routineValues.size() < flatPoints.size()) {
            routineValues.resize(flatPoints.size(), 0);
        }

        LoggerStream::getInstance() << DEBUG << "Task " << name << " has " << flatPoints.size() << " points and " << routineValues.size() << " routine values";

        
        for (int i = 0; i < flatPoints.size(); i++) {            
            vec.push_back(make_shared<TaskPoint>(*flatPoints[i], routineValues[i]));
        }
        
        TaskPointVector geometries(vec);
        this->points = geometries;
    } else { // continous or hitch task --> save as polygons
        geometryType = GeometryType::POLYGONS;
        vector<TaskPolygonPtr> vec;
        for (int i = 0; i < f.getNumSeries(); i++) {
            try { 
                vec.push_back(make_shared<TaskPolygon>(f.getPoints(i), f.getFieldByName<int>(i, "rate")));
            } catch (std::runtime_error& e) {
                vec.push_back(make_shared<TaskPolygon>(f.getPoints(i), 0));
                LoggerStream::getInstance() << WARN << "No rate field of type int found for polygon " << i << " in task " << name << ", defaulting to 1";
                LoggerStream::getInstance() << WARN << "Error: " << e.what();
            }
        }
        TaskPolygonVector geometries(vec);
        this->polygons = geometries;
    }
}

Eigen::Affine3d Task::getDiscreteReference()
{
    if (getImplement().getSections().size() > 0) {
        return getImplement().getSections().at(0)->getState().asAffine();
    } else {
        return getHitch().getState().asAffine();
    }
}

double Task::distanceToNextDiscrPoint(const IndexPoint& closestTrajectPoint, double interpolationDistance) 
{
    int s = getPathPointsDiscr().size();
    if (s > 0) { // only if discrete task
        // if the last index is already passed do not increment points
        if (nextDiscreteImplementIndex < s) {
            int numberOfPoints = getPathPointsDiscr().at(nextDiscreteImplementIndex)->index - closestTrajectPoint.index;
            return interpolationDistance * numberOfPoints;
        }
    } 
    // return large value to illustrate there is no approaching point
    return 1e6;
}

void Task::incrDiscrPoint()
{
    int s = getPathPointsDiscr().size();
    if (s > 0) { // only if discrete task
        nextDiscreteImplementIndex++;
        if (nextDiscreteImplementIndex < (s-1) ) {
            LoggerStream::getInstance() << DEBUG << "new idx is: " << nextDiscreteImplementIndex << " and in path: " << getPathPointsDiscr()[nextDiscreteImplementIndex]->index;
        } else {
            LoggerStream::getInstance() << DEBUG << "last point is finished!";
        }
    }
}


void Task::onDiscrReset(const IndexPoint& closestTrajectPoint, const vector<PointPtr>& interpolation)
{
    if (getGeometry<TaskPointVector>().size() > 0) { // only if discrete task
        createPathPointsDiscr(interpolation);
        printRapport(LoggerStream::getInstance());
        nextDiscreteImplementIndex = 0;
        int s = getPathPointsDiscr().size();
        int i = 0;
        while (i < s) {
            if (closestTrajectPoint.index < getPathPointsDiscr().at(i)->index) {
                break;
            }
            i++;
        }
        if (i < s) {
            nextDiscreteImplementIndex = i;
        } else {
            nextDiscreteImplementIndex = s-1;
        }
        
        LoggerStream::getInstance() << DEBUG << "RESET -- closestTrajectPoint.index: " << closestTrajectPoint.index << ", nextDiscreteImplementIndex: " << nextDiscreteImplementIndex;
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
    for (TaskPointPtr p_task: get<TaskPointVector>(points)) {
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
        discretePathPoints.push_back(make_shared<IndexPoint>(p));
    }
    sort(discretePathPoints.begin(), discretePathPoints.end(), this->compareClosePoints);
    discretePathPoints.erase(unique(discretePathPoints.begin(), discretePathPoints.end(), this->equalClosePoints), discretePathPoints.end());
}

void Task::printRapport(LoggerStream& logger) 
{
    stringstream s;
    s << "** Task rapports" << endl;
    if (geometryType == POLYGONS) {
        s << "## Polygon Array ##" << endl;
    
        TaskPolygonVector& polyVec = get<TaskPolygonVector>(polygons);
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
        TaskPointVector pointVec = get<TaskPointVector>(points);
        s << "## Point array ##" << endl;
        s << setprecision(15);
        TablePrinter tp(&s);
        if (discretePathPoints.size() > 0) {
            tp.AddColumn("idx", 10);
            tp.AddColumn("X_path [m]", 15);
            tp.AddColumn("Y_path [m]", 15);
            tp.PrintHeader();
            for (int i = 0; i < discretePathPoints.size(); i++) {
                tp << discretePathPoints.at(i)->index << discretePathPoints.at(i)->x() << discretePathPoints.at(i)->y();
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
    return discretePathPoints;
}

bool Task::updateSections(VariableManager* manager, bool disable)
{
    bool activeSections = false;

    for (int i = 0; i < implement.getSections().size(); i++) {
        auto section = implement.getSections().at(i);
        string name = "plc.control." + hitch.getEntityName() + ".activate_sections." + to_string(i);
        if (getImplement().worksOnTaskmap()) {
            section->setRate(getTaskMapRate(section, disable));
            manager->getVariable(name)->setValue<int>((int) section->getRate());
        } else {
            bool active = manager->getVariable(name)->getValue<bool>();
            section->setRate(active ? 1 : 0);
        }
        if (section->getRate()) {
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
            section->setRate(value ? 1 : 0);
        }
    }
}

bool Task::insideTaskMap(shared_ptr<Section> section, bool disable)
{
    return insideTaskMap(section, disable) != 0;
}

int Task::getTaskMapRate(shared_ptr<Section> section, bool disable)
{
    const Polygon& polygonSection = section->getPolygon();
    Point currentPosition(section->getState().getT().asVector());
    section->clearActivationGeometry();

    if (type.compare("continuous") == 0) { 
        for (TaskPolygonPtr polygon: get<TaskPolygonVector>(polygons)) { 
            if (overlaps(polygonSection.geometry(), polygon->geometry()) || covered_by(polygonSection.geometry(), polygon->geometry())) {
                section->setActivationGeometry(polygon);
                return disable ? 0 : static_cast<uint8_t>(polygon->getRate());
            }
        }     
    } else if (type.compare("cardan") == 0) {
        for (TaskPolygonPtr polygon: get<TaskPolygonVector>(polygons)) { 
            if (overlaps(polygonSection.geometry(), polygon->geometry()) || covered_by(polygonSection.geometry(), polygon->geometry())) {
                // section->setActivationGeometry(polygon);
                return disable ? 0 : 1;
            }
        } 
    } else if (type.compare("intermittent") == 0) {
        TaskPointVector vec = get<TaskPointVector>(points);
        std::vector<TaskPointPtr> points = vec.nearby(currentPosition, 40);
        for (TaskPointPtr point: points) {
            if (covered_by(point->geometry(), polygonSection.geometry())) {
                section->addActivationGeometry(point);
                return disable ? 0 : 1;
            }
        }   
    }

    return false;
}


int Task::getTaskMapRoutine()
{
    int routine = getGeometry<TaskPointVector>().at(nextDiscreteImplementIndex)->routine;
    return routine;
}


bool Task::insideTaskMap(Point point, bool disable)
{
    TaskPolygonVector& vec = get<TaskPolygonVector>(polygons);
    if (vec.size() > 0) {
        for (TaskPolygonPtr polygon: vec) { 
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
        TaskPolygonVector polyVec = get<TaskPolygonVector>(polygons);
        for (TaskPolygonPtr polygonPtr: polyVec) {
            j_points.push_back(polygonPtr->toJson());
        }
    } else {
        TaskPointVector pointVec = get<TaskPointVector>(points);
        j_points.push_back(json::array());  // same look as polygon array
        for (TaskPointPtr pointPtr: pointVec) {
            j_points[0].push_back(pointPtr->toJson());
        }
    }


    j["name"] = this->name;
    j["hitch"] = this->hitch.name;
    j["type"] = this->type;
    j["points"] = j_points;

    return j;
}