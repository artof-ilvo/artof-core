/**
 * @file Task.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Task to be executed loaded from the field configuration files
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <climits>
#include <ThirdParty/json.hpp>
#include <Utils/Geometry/Polygon.h>
#include <Utils/Geometry/GeometryVector.h>
#include <Utils/Geometry/Point.h>
#include <Utils/Settings/Platform.h>
#include <Utils/Settings/Implement.h>
#include <Utils/File/PointData.h>
#include <Utils/Redis/VariableManager.h>

namespace Ilvo {
namespace Utils {
namespace Settings {

    enum GeometryType {POINTS, POLYGONS};

    /**
     * @brief Next measurement point of a discrete implement
     * 
     */
    class NextDiscrImplPoint
    {
    public:
        Geometry::PointPtr implPoint;
        int idx;
        Geometry::IndexPoint closest_path_point;
    
        NextDiscrImplPoint() : implPoint(nullptr), idx(0) {}
        ~NextDiscrImplPoint() = default;
    };

    class Task
    {
    private:
        Utils::Settings::Platform& platform;
        int gpsZoneId;
        std::string name;
        std::string type;
        Hitch& hitch;  // keep reference to same hitch instance as in platform
        Implement implement;

        std::string taskmappath;
        GeometryType geometryType;
        std::variant<Geometry::TaskPolygonVector,Geometry::TaskPointVector> polygons, points;

        int nextDiscreteImplementIndex;
        std::vector<Geometry::IndexPointPtr> discretePathPoints;

        void initVariant(Utils::File::PointData& f);
        static bool compareClosePoints(Geometry::IndexPointPtr p1, Geometry::IndexPointPtr p2);
        static bool equalClosePoints(Geometry::IndexPointPtr p1, Geometry::IndexPointPtr p2);

    public:
        Task(std::string baseFilePath, nlohmann::json task, int gpsZoneId);
        ~Task() = default;

        void updateState(Redis::VariableManager* manager);
        bool updateSections(Redis::VariableManager* manager, bool disable=false);
        bool cardanEnabled(Redis::VariableManager* manager, bool disable=false);
        void activateSection(std::string id, bool value);

        bool hitchInTaskMap();
        bool insideTaskMap(Geometry::Point point, bool disable=false);
        bool insideTaskMap(std::shared_ptr<Section> section, bool disable=false);
        int getTaskMapRate(std::shared_ptr<Section> section, bool disable=false);
        int getTaskMapRoutine();

        Hitch& getHitch();
        bool onHitch(std::string name);

        Implement& getImplement();

        const std::string& getType();
        bool isType(std::string type);
        const std::string& getName();

        void createPathPointsDiscr(std::vector<Geometry::PointPtr> trajectPoints);
        const std::vector<Geometry::IndexPointPtr>& getPathPointsDiscr();
        
        const GeometryType& getGeometryType();
        template<class T>
        const T& getGeometry()
        {
            if (geometryType == GeometryType::POLYGONS) return std::get<T>(polygons);
            else return std::get<T>(points);
        }

        // returns the distance to the next discrete measurement point 
        double distanceToNextDiscrPoint(const Geometry::IndexPoint& closestTrajectPoint, double interpolationDistance);
        // get discrete reference
        Eigen::Affine3d getDiscreteReference();
        // increments the discrete measurement point, for the next point to drive to 
        void incrDiscrPoint();
        // resets the discrete measurement point, the closest discrete measurement point will be executed next 
        void onDiscrReset(const Geometry::IndexPoint& closestTrajectPoint, const std::vector<Geometry::PointPtr>& interpolation);

        void printRapport(Utils::Logging::LoggerStream& logger);
        nlohmann::json toJson() const;
    };

    inline std::ostream & operator<<(std::ostream& os, const Task& t) { 
        os << t.toJson().dump();
        return os;
    }

} // namespace Ilvo
} // namespace Utils
} // namespace Robot
