/**
 * @file Field.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Field description stored in the file hierarchy
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <Utils/Settings/Task.h>
#include <ThirdParty/json.hpp>
#include <Utils/Geometry/Polygon.h>
#include <Utils/Geometry/Point.h>

namespace Ilvo {
namespace Utils {
namespace Settings {

    /** @brief Abstraction of a field */
    class Field
    {
    public:
        std::string trajectFilePath;
        std::string geofenceFilePath;
        std::string infoFilePath;
    private:
        std::string baseFilePath;
        std::string name;
        /** @brief The GPS zone id of the field (UTM...N) */
        int gpsZoneId;

        /** @brief The field information that is also stored in the file hierarchy as a json */
        nlohmann::json fieldInfo;
        /** @brief A list of tasks in the field */
        std::vector<Task> tasks;

        /** @brief The geofence of the field (the robot cannot operate outside this geofence)*/
        Geometry::Polygon geofence;
        /** @brief A list of the points the traject (not interpolated)*/
        std::vector<Geometry::PointPtr> trajectPoints;
    public:
        Field();
        Field(std::string name, int zoneId);
        ~Field() = default;

        Field& operator=(const Field& other);

        const std::vector<Geometry::PointPtr>& getTrajectPoints() const;
        const Geometry::Polygon& getGeofence() const;
        std::vector<Task>& getTasks();
        /** @brief Check if the field has tasks of a certain type */
        bool hasTasksWithType(std::string type);
        /** @brief Check if the field has tasks of a certain type on the given hitch */
        bool hasTaskWithTypeOnHitch(std::string type, std::string hitchName);
        /** @brief Get the task of a certain type */
        Task& getTaskWithType(std::string type);
        /** @brief Get the field as a json object */
        Task& getTaskWithTypeOnHitch(std::string type, std::string hitchName);

        nlohmann::json toJson() const;
        /** @brief Write the field to a json file */
        void writeJson(const std::string& relativePath) const;
        void writeJson() const;

        /** @brief Get the name of the active field (extracted from Redis server) */
        static std::string checkFieldName(std::string name);
    };

    inline std::ostream& operator<<(std::ostream& os, const Field& f) {
        os << f.toJson().dump();
        return os;
    }

} // namespace Ilvo
} // namespace Utils
} // namespace Robot

