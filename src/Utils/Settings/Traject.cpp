#include <Utils/Settings/Traject.h>
#include <Utils/Geometry/Transform.h>
#include <Utils/Geometry/GeometryVector.h>
#include <Utils/File/File.h>
#include <Utils/Logging/LoggerStream.h>
#include <Utils/File/PointShapeFile.h>
#include <Utils/File/PointCsvFile.h>
#include <ThirdParty/json.hpp>
#include <boost/filesystem.hpp>
#include <Exceptions/FileExceptions.hpp>
#include <Exceptions/RobotExceptions.hpp>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::File;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Logging;

using namespace std;
using namespace nlohmann;
using namespace boost::filesystem;
using namespace boost::geometry;

Traject::Traject() : 
    loaded(false)
{
}


void Traject::load(std::string fieldName, int utmZoneId, double cornerDetectionAngle, double interpolationDistance, double turnRadius) 
{
    // load the field
    field = std::make_shared<Field>(fieldName, utmZoneId);

    // Doing the initialization
    if (field->getTrajectPoints().size() > 1 && interpolationDistance > 0 && cornerDetectionAngle > 0) {

        // helper objects
        Line l1, l2;

        // ** Clear all old data
        // clear corners and add first point
        corners.clear();
        int cornerIndex = 0;
        corners.push_back(make_shared<CornerPoint>(0, *getRawPoints().at(0), 0, cornerIndex, Point(), *getRawPoints().at(1)));
        cornerIndex++;
        LoggerStream::getInstance() << DEBUG << *corners.back();

        // clear interpolation
        interpolation.clear();
        // clear skeleton
        skeleton.clear();

        // ** Detect Corners and linear interpolation
        int i = 0;
        while (i < rawPointsLength()-1) {
            PointPtr p1 = getRawPoints().at(i);
            PointPtr p2 = getRawPoints().at(i+1);
            l1 = Line(*p1, *p2);

            // Add point to skeleton
            skeleton.push_back(p1);
            // Add interpolated line
            auto newInterpolationData = l1.interpolate(interpolationDistance);
            interpolation.insert(interpolation.end(), newInterpolationData.begin(), newInterpolationData.end());

            // corner detection
            if (i < rawPointsLength()-2) {
                PointPtr p3 = getRawPoints().at(i+2);
                l2 = Line(*p2, *p3);

                // determine corner angle
                bool cornerDetection = false;
                double cornerAngle = 0.0;
                // Only calculate the corner size if the lengths are valid
                if (l1.length() > 0 && l2.length() > 0) {
                    cornerAngle = l1.corner(l2);
                    if (cornerAngle <= 90) cornerDetection = cornerAngle > cornerDetectionAngle; 
                    else cornerDetection = cornerAngle < (180 - cornerDetectionAngle);
                }

                if (cornerDetection) {
                    corners.push_back(make_shared<CornerPoint>(interpolation.size()-1, *p2, cornerAngle, cornerIndex, *p1, *p3));
                    cornerIndex += 1;
                    // headland detection
                    if (i < rawPointsLength()-3) {
                        PointPtr p4 = getRawPoints().at(i+3);
                        Line l3(*p3, *p4);

                        double pathAngleDifference = calcSmallestAngleAbsolute(l1.alpha(), l3.alpha());
                        double distance = p2->distance(*p3);
                        if (distance < 2 * turnRadius && inRange(180 - 10, 180 + 10, pathAngleDifference)) { 
                            corners.back()->setHeadland(distance);
                        }
                    }
                    LoggerStream::getInstance() << DEBUG << *corners.back();
                }
            }         

            // update i
            i++;
        }

        // add last point to skeleton
        skeleton.push_back(getRawPoints().at(i));
        // add last point as corner
        l1 = Line(getRawPoints().at(i-1), getRawPoints().at(i));
        corners.push_back(make_shared<CornerPoint>(interpolation.size()-1, *getRawPoints().at(i), 0, cornerIndex, *getRawPoints().at(i-1), Point()));
        LoggerStream::getInstance() << DEBUG << *corners.back();

        LoggerStream::getInstance() << INFO << "Loaded traject with: ";
        LoggerStream::getInstance() << INFO << "- " << getRawPoints().size() << " field traject points";
        LoggerStream::getInstance() << INFO << "- " << interpolation.size() << " interpolated points";
        LoggerStream::getInstance() << INFO << "- " << skeleton.size() << " skeleton points";
        LoggerStream::getInstance() << INFO << "- " << corners.size() << " corners";

        loaded = true;
    } else {
        loaded = false;
    }
}

const bool Traject::empty() const
{
    return !loaded;
}

const vector<PointPtr>& Traject::getRawPoints() const
{ 
    return field->getTrajectPoints(); 
}

const vector<PointPtr>& Traject::getInterpolation() const
{ 
    return interpolation;
}

const vector<CornerPointPtr>& Traject::getCorners() const
{ 
    return corners; 
}

Field& Traject::getField()
{
    return *field;
}

int Traject::interpolationLength() 
{
    return int(interpolation.size());
}

int Traject::cornersLength() 
{
    return int(getCorners().size());
}

int Traject::rawPointsLength() 
{
    return int(getField().getTrajectPoints().size());
}

IndexPoint Traject::closestPoint(Point currentPoint) 
{
    double mindist = 1e6;
    int minindex = 0;

    for (int i = 0; i < interpolationLength(); i++) {
        double dist = interpolation.at(i)->distance(currentPoint);

        // find the closest point
        if (dist < mindist) {
            mindist = dist;
            minindex = i;
        }
    }

    return IndexPoint(*interpolation.at(minindex), minindex);
}

IndexPoint Traject::closestPoint(Point currentPoint, int startIdx, int endIdx, double distance) 
{
    double mindist = 1e6;
    int minIndex = startIdx;
    int interpolationLength = this->interpolationLength();

    for (int i = startIdx; i < endIdx; i++) {
        if (i < interpolationLength) {
            Point p_path = *interpolation.at(i);

            double dist = p_path.distance(currentPoint);

            // find the closest point
            if (abs(dist-distance) < mindist) {
                mindist = abs(dist-distance);
                minIndex = i;
            }
        }
    }

    // Assert when the minIndex is larger than the interpolationLength
    if (minIndex >= interpolationLength) {
        throw TrajectIndexException(minIndex, interpolationLength);
    }

    return IndexPoint(*interpolation.at(minIndex), minIndex);
}

CornerPoint Traject::cornerByPathIndex(int indexInPath) 
{
    for (int i = 1; i < cornersLength(); i++) { // first one is starting of the Traject
        if ((*corners[i]).index > indexInPath) {
            return *corners[i];
        }
    }
    // give first point if no other is found --> may not occur
    return *corners[0];
}

CornerPoint Traject::cornerByIndex(int cornerIndex)
{
    if (cornerIndex < 0){
        return *corners[0];
    } else if (cornerIndex >= cornersLength()) {
        return *corners[cornersLength()-1];
    } else {
        return *corners[cornerIndex];
    }
}

NextAndPreviousCorner Traject::cornersByPathIndex(int indexInPath)
{
    if (!loaded) {
        throw TrajectNotLoadedException();
    }

    for (int i = 1; i < cornersLength(); i++) { // first one is starting of the Traject
        if (corners[i]->index > indexInPath) {
            return NextAndPreviousCorner(*corners[i-1], *corners[i]);
        }
    }
    // give first point if no other is found --> may not occur
    return NextAndPreviousCorner(*corners[0], *corners[1]);
}

NextAndPreviousCorner Traject::cornersByIndex(int cornerIndex) 
{
    if (cornerIndex <= 0) { 
        // return first one
        return NextAndPreviousCorner(*corners[0], *corners[1]);
    } else if (cornerIndex >= cornersLength() - 1) { 
        // return last one
        return NextAndPreviousCorner(*corners[cornersLength()-2], *corners[cornersLength()-1]);
    } 
    
    // return the one
    return NextAndPreviousCorner(*corners[cornerIndex-1], *corners[cornerIndex]);
}

Line Traject::pathLine(int idx) 
{
    Line line;
    // if idx is last point take point in front of it
    if (idx >= interpolationLength()-1) 
    {
        line = Line(*interpolation.at(interpolationLength()-2), *interpolation.at(interpolationLength()-1));
    }
    else // take point after this point
    {
        line = Line(*interpolation.at(idx), *interpolation.at(idx+1));
    }
    return line;
}

int Traject::isPointLeft(int index, Point& point)
{
    Line line = pathLine(index);
    return line.left(point) ? -1.0 : 1.0; 
}

double Traject::absPathOrientation(int idx) 
{
    return pathLine(idx).alpha();
}

bool Traject::outsideGeofence(Point point)
{
    return !covered_by(point.geometry(), field->getGeofence().geometry());
}

bool Traject::insideFirstTask(Point point)
{
    if (field->getTasks().size() > 1) {
        Task& task = field->getTasks()[0];
        return task.insideTaskMap(point);
    }
    return false;
}

bool Traject::insideAnyTask(Point point)
{
    for (Task& task: field->getTasks()) {
        if(task.insideTaskMap(point)) {
            return true;
        }
    }
    return false;
}

json Traject::toJson() const
{
    json j = field->toJson();

    json skeleton = json::array();
    for (auto pointPtr: this->skeleton) {
        skeleton.push_back(pointPtr->toJson());
    }

    json jCorners = json::array();
    for (auto cornerPtr: this->corners) {
        jCorners.push_back(cornerPtr->point.toJson());
    }

    j["skeleton"] = skeleton;
    j["corners"] = jCorners;

    return j;
}
