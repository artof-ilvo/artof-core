/**
 * @file Traject.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Manages the interpolation of the traject and the associated fields
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept> // std::runtime_error
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
#include <Utils/Geometry/Point.h>
#include <Utils/Geometry/Line.h>
#include <Utils/Geometry/Angle.h>
#include <Utils/Settings/Field.h>
#include <Utils/Settings/Navigation.h>

namespace Ilvo {
namespace Utils {
namespace Settings {

    /**
     * @brief traject class: manages the traject or traject (e.g. loading it from the shape files, the interpolation, ...)
     * 
     */
    class Traject
    {
    private:
        bool loaded;
        std::shared_ptr<Field> field;

    private:
        InterpolationType interpolationType;
        /** @brief list of interpolated points of the traject */
        std::vector<Geometry::PointPtr>* interpolation;
        /** @brief list of interpolated points of the traject */
        std::vector<Geometry::PointPtr> interpolationLinear;
        /** @brief list of interpolated points of the curvy traject */
        std::vector<Geometry::PointPtr> interpolationCurvy;
        /** @brief list of skeleton points of the traject */
        std::vector<Geometry::PointPtr>* skeleton;
        /** @brief list of skeleton points of the traject */
        std::vector<Geometry::PointPtr> skeletonLinear;
        /** @brief list of skeleton points of the curvy traject */
        std::vector<Geometry::PointPtr> skeletonCurvy;
        /** @brief list of corners of the traject */
        std::vector<Geometry::CornerPointPtr> corners;

        friend std::ostream& operator<<(std::ostream& os, const Traject& t);
    public:
        Traject();
        ~Traject() = default;

        void load(std::string fieldName, int utmZoneId, 
                double cornerDetectionAngle=15.0, double interpolationDistance=0.1, 
                double turnRadius=6.0, InterpolationType type=InterpolationType::LINEAR);
        const bool empty() const;

        const std::vector<Geometry::PointPtr>& getRawPoints() const;
        const std::vector<Geometry::CornerPointPtr>& getCorners() const;
        Field& getField();

        const std::vector<Geometry::PointPtr>& getInterpolation(InterpolationType type=InterpolationType::CURRENT) const;
        InterpolationType getInterpolationType() const;
        void setInterpolation(InterpolationType type);

        int rawPointsLength();
        int interpolationLength();
        int cornersLength();

        /** @brief returns the closest point in the traject to the current point*/
        Geometry::IndexPoint closestPoint(Geometry::Point currentPoint);
        /** @brief returns the closest point in the traject to the current point in the range [startIdx, endIdx]*/
        Geometry::IndexPoint closestPoint(Geometry::Point currentPoint, int startIdx, int endIdx, double distance=0.0);
        /** @brief returns the next corner in the traject on the index */
        Geometry::CornerPoint cornerByPathIndex(int indexInPath);
        /** @brief returns the next corner in the traject on the index in the range [startIdx, endIdx]*/
        Geometry::CornerPoint cornerByIndex(int cornerIndex);
        /** @brief returns the next and previous corner relative to a specific interpolated point in the traject */
        Geometry::NextAndPreviousCorner cornersByPathIndex(int indexInPath);
        /** @brief returns the next and previous corner relative to a specific corner in the traject */
        Geometry::NextAndPreviousCorner cornersByIndex(int cornerIndex);

        /** @brief returns the orientation of the path in the traject on the index*/
        double absPathOrientation(int idx);
        /** @brief returns the line of the path in the traject on the index*/
        Geometry::Line pathLine(int idx);
        /** 
         * @brief returns 1 if the point is left to the line in the traject on the index
         * 
         * @param point: a point
         * @return 1 if the point is left of the line
        */
        int isPointLeft(int index, Geometry::Point& point);
        /** @brief checks if the point is outside the geofence */
        bool outsideGeofence(Geometry::Point point);
        /** @brief checks if the point is inside the first task */
        bool insideFirstTask(Geometry::Point point);
        /** @brief checks if the point is inside any task */
        bool insideAnyTask(Geometry::Point point);
        /** @brief returns the distance to the next discrete measurement point */
        double distanceToNextDiscrPoint(Task& task, double interpolationDistance);
        /** @brief increments the discrete measurement point, for the next point to drive to */
        void incrDiscrPoint(Task& task);
        /** @brief resets the discrete measurement point, the closest discrete measurement point will be executed next */
        void onDiscrReset(Geometry::Point point);

        nlohmann::json toJson() const;
    };

    inline std::ostream& operator<<(std::ostream& os, const Traject& t) {
        os << t.toJson().dump();
        return os;
    }

} // namespace Ilvo
} // namespace Utils
} // namespace Robot
