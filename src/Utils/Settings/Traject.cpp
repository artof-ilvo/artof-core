#include <Utils/Settings/Traject.h>
#include <Utils/Geometry/Transform.h>
#include <Utils/Geometry/GeometryVector.h>
#include <Utils/File/File.h>
#include <Utils/Logging/LoggerStream.h>
#include <Utils/File/PointShapeFile.h>
#include <Utils/File/PointCsvFile.h>
#include <Utils/Geometry/Arc.h>
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


void Traject::load(std::string fieldName, int utmZoneId, double cornerDetectionAngle, double interpolationDistance, double turnRadius, InterpolationType type) 
{
    // load the field
    field = std::make_shared<Field>(fieldName, utmZoneId);

    // Doing the initialization
    if (field->getTrajectPoints().size() > 1 && interpolationDistance > 0 && cornerDetectionAngle > 0) {

        // helper objects
        Line l1, l2;
        Arc a;

        // ** Clear all old data
        // clear corners and add first point
        corners.clear();
        int cornerIndex = 0;
        corners.push_back(make_shared<CornerPoint>(0, *getRawPoints().at(0), 0, cornerIndex, Point(), *getRawPoints().at(1)));
        cornerIndex++;
        LoggerStream::getInstance() << DEBUG << *corners.back();

        // clear interpolation linear
        interpolationLinear.clear();
        // clear interpolationCurvy
        interpolationCurvy.clear();

        // clear skeletonLinear
        skeletonLinear.clear();
        // clear skeletonCurvy
        skeletonCurvy.clear();

        // ** Detect Corners and linear interpolation
        int i = 0;
        while (i < rawPointsLength()-1) {
            PointPtr p1 = getRawPoints().at(i);
            PointPtr p2 = getRawPoints().at(i+1);
            l1 = Line(*p1, *p2);

            // Add point to skeleton
            skeletonLinear.push_back(p1);
            // Add interpolated line
            auto newInterpolationData = l1.interpolate(interpolationDistance);
            interpolationLinear.insert(interpolationLinear.end(), newInterpolationData.begin(), newInterpolationData.end());

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
                    corners.push_back(make_shared<CornerPoint>(interpolationLinear.size()-1, *p2, cornerAngle, cornerIndex, *p1, *p3));
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
        skeletonLinear.push_back(getRawPoints().at(i));
        // add last point as corner
        l1 = Line(getRawPoints().at(i-1), getRawPoints().at(i));
        corners.push_back(make_shared<CornerPoint>(interpolationLinear.size()-1, *getRawPoints().at(i), 0, cornerIndex, *getRawPoints().at(i-1), Point()));
        LoggerStream::getInstance() << DEBUG << *corners.back();

        // ** Interpolation Curvy
        if (cornersLength() == 2) {
            // If one line just add the interpolated line
            l1 = Line(corners.at(0)->point, corners.at(1)->point);
            interpolationCurvy = l1.interpolate(interpolationDistance, true);
        } else {
            // iterate over new corners
            vector<PointPtr> interpolationCurvySkeleton;
            interpolationCurvySkeleton.push_back(make_shared<Point>(corners.at(0)->point));

            double arcInterpolationDistance = 0.5;
            int i = 1;
            while (i < cornersLength()) { 
                CornerPointPtr c1 = corners.at(i);

                if (c1->nextRawPoint.isEmpty()) { // Last point
                    interpolationCurvySkeleton.push_back(make_shared<Point>(c1->point));
                    // update i
                    i += 1;
                } else if (c1->isHeadland) { // It is a headland
                    a = Arc(c1->point, c1->nextRawPoint, c1->previousRawPoint, turnRadius);
                    vector<PointPtr> aInterpolation = a.interpolate(arcInterpolationDistance);

                    interpolationCurvySkeleton.insert(interpolationCurvySkeleton.end(), aInterpolation.begin(), aInterpolation.end());
                    // update i
                    i += 2;  // Skip the next point
                } else {  // It is a normal corner
                    l1 = Line(c1->previousRawPoint, c1->point);
                    l2 = Line(c1->point, c1->nextRawPoint);
                    a = Arc(l1, l2, turnRadius);
                    vector<PointPtr> aInterpolation = a.interpolate(arcInterpolationDistance);

                    interpolationCurvySkeleton.insert(interpolationCurvySkeleton.end(), aInterpolation.begin(), aInterpolation.end());
                    // update i
                    i += 1;
                }
            }

            // Clean other directions
            skeletonCurvy.push_back(interpolationCurvySkeleton.at(0)); // Add first point
            skeletonCurvy.push_back(interpolationCurvySkeleton.at(1)); // Add second point

            i = 2;
            while (i < interpolationCurvySkeleton.size()) {
                PointPtr p1 = skeletonCurvy.at(skeletonCurvy.size()-2);
                PointPtr p2 = skeletonCurvy.at(skeletonCurvy.size()-1);
                PointPtr pNew = interpolationCurvySkeleton.at(i);
                l1 = Line(*p1, *p2);
                l2 = Line(*p2, *pNew);
                double angle = l1.corner(l2);
                // remove sharp angles or lines that are going backwards
                if (angle > 90 && l2.length() > interpolationDistance) { 
                    CurvyPoint* pCurvy = dynamic_cast<CurvyPoint*>(pNew.get());
                    double radius = (pCurvy != nullptr) ? pCurvy->radius : 0.0;
                    skeletonCurvy.push_back(make_shared<CurvyPoint>(pNew->x(), pNew->y(), radius));
                } 

                // update i
                i += 1;
            }

            // add points to interpolation curvy
            i = 0;
            while (i < skeletonCurvy.size()-1)
            {
                PointPtr p1 = skeletonCurvy.at(i);
                PointPtr p2 = skeletonCurvy.at(i+1);
                CurvyPoint* p1Curvy = dynamic_cast<CurvyPoint*>(p1.get());
                l1 = Line(p1, p2);
                double radius = (p1Curvy != nullptr && l1.length() < arcInterpolationDistance + 0.01) ? p1Curvy->radius : 0.0;
                for (PointPtr p : l1.interpolate(interpolationDistance)) {
                    interpolationCurvy.push_back(make_shared<CurvyPoint>(p->x(), p->y(), radius));
                }
                // update i
                i += 1;
            }
            
        }

        LoggerStream::getInstance() << INFO << "Loaded traject with: ";
        LoggerStream::getInstance() << INFO << "- " << getRawPoints().size() << " field traject points";
        LoggerStream::getInstance() << INFO << "- " << interpolationLinear.size() << " interpolated points";
        LoggerStream::getInstance() << INFO << "- " << skeletonLinear.size() << " skeleton linear points";
        LoggerStream::getInstance() << INFO << "- " << interpolationCurvy.size() << " interpolated curvy points";
        LoggerStream::getInstance() << INFO << "- " << skeletonCurvy.size() << " skeleton curvy points";
        LoggerStream::getInstance() << INFO << "- " << corners.size() << " corners";

        setInterpolation(type);
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

const vector<PointPtr>& Traject::getInterpolation(InterpolationType type) const
{ 
    if (type == InterpolationType::CURRENT) {
        return *interpolation;
    } else if (type == InterpolationType::LINEAR) {
        return interpolationLinear;
    } else {
        return interpolationCurvy;
    }
}

InterpolationType Traject::getInterpolationType() const
{
    return interpolationType;
}

void Traject::setInterpolation(InterpolationType type)  
{
    interpolationType = type;
    if (interpolationType == InterpolationType::CURVY) {
        interpolation = &interpolationCurvy;
        skeleton = &skeletonCurvy;
    } else {
        interpolation = &interpolationLinear;
        skeleton = &skeletonLinear;
    }
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
    return int(interpolation->size());
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
        double dist = interpolation->at(i)->distance(currentPoint);

        // find the closest point
        if (dist < mindist) {
            mindist = dist;
            minindex = i;
        }
    }

    return IndexPoint(*interpolation->at(minindex), minindex);
}

IndexPoint Traject::closestPoint(Point currentPoint, int startIdx, int endIdx, double distance) 
{
    double mindist = 1e6;
    int minIndex = startIdx;

    for (int i = startIdx; i < endIdx; i++) {
        if (i < interpolationLength()) {
            Point p_path = *interpolation->at(i);

            double dist = p_path.distance(currentPoint);

            // find the closest point
            if (abs(dist-distance) < mindist) {
                mindist = abs(dist-distance);
                minIndex = i;
            }
        }
    }

    // Assert when the minIndex is larger than the interpolationLength
    if (minIndex >= interpolationLength()) {
        throw TrajectIndexException(minIndex, interpolationLength());
    }

    return IndexPoint(*interpolation->at(minIndex), minIndex);
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
        line = Line(*interpolation->at(interpolationLength()-2), *interpolation->at(interpolationLength()-1));
    }
    else // take point after this point
    {
        line = Line(*interpolation->at(idx), *interpolation->at(idx+1));
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

double Traject::distanceToNextDiscrPoint(Task& task, double interpolationDistance) 
{
    int s = task.getPathPointsDiscr().size();
    if (s > 0) { // only if discrete task
        IndexPoint closestPointToCurrentPosition;
        if (task.getImplement().getSections().size() > 0) {
            closestPointToCurrentPosition = closestPoint(task.getImplement().getSections().at(0)->getState().asAffine());
        } else {
            closestPointToCurrentPosition = closestPoint(task.getHitch().getState().asAffine());
        }
        // if the last index is already passed do not increment points
        if (task.nextDiscreteImplementIndex < s) {
            int numberOfPoints = task.getPathPointsDiscr().at(task.nextDiscreteImplementIndex)->index - closestPointToCurrentPosition.index;
            return interpolationDistance * numberOfPoints;
        }
    } 
    // return large value to illustrate there is no approaching point
    return 1e6;
}

void Traject::incrDiscrPoint(Task& task)
{
    int s = task.getPathPointsDiscr().size();
    if (s > 0) { // only if discrete task
        task.nextDiscreteImplementIndex++;
        if (task.nextDiscreteImplementIndex < (s-1) ) {
            LoggerStream::getInstance() << DEBUG << "new idx is: " << task.nextDiscreteImplementIndex << " and in path: " << task.getPathPointsDiscr()[task.nextDiscreteImplementIndex]->index;
        } else {
            LoggerStream::getInstance() << DEBUG << "last point is finished!";
        }
    }
}


void Traject::onDiscrReset(Point point)
{
    for (Task& task: field->getTasks()) {
        if (task.getGeometry<PointVector>().size() > 0) { // only if discrete task
            IndexPoint closestPointToCurrentPosition = closestPoint(point);
            task.createPathPointsDiscr(*this->interpolation);
            task.printRapport(LoggerStream::getInstance());
            task.nextDiscreteImplementIndex = 0;
            int s = task.getPathPointsDiscr().size();
            int i = 0;
            while (i < s) {
                if (closestPointToCurrentPosition.index < task.getPathPointsDiscr().at(i)->index) {
                    break;
                }
                i++;
            }
            if (i < s) {
                task.nextDiscreteImplementIndex = i;
            } else {
                task.nextDiscreteImplementIndex = s-1;
            }
            
            LoggerStream::getInstance() << DEBUG << "RESET -- closestPointToCurrentPosition.index: " << closestPointToCurrentPosition.index << ", task.nextDiscreteImplementIndex: " << task.nextDiscreteImplementIndex;
        }
    }
}

json Traject::toJson() const
{
    json j = field->toJson();

    json skeleton = json::array();
    for (auto pointPtr: *this->skeleton) {
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
