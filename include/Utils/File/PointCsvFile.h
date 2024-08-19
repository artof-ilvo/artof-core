/**
 * @file PointCsvFile.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Extract PointData from a csv file
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <chrono>
#include <sys/time.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <Utils/Geometry/Point.h>
#include <Utils/File/PointData.h>

namespace Ilvo {
namespace Utils {
namespace File {

    /**
     * @brief Loads a csv file and converts it to PointData.
     * 
     */
    class PointCsvFile : public PointData
    {
    private:
        std::string csv_file;
        std::map<std::string, int> column_names;
        std::vector<std::vector<std::string>> data_lines;
        
        void readPointFile(std::vector<std::string>& xFields, std::vector<std::string>& yFields);
    public:
        void init(const std::string& filename, std::vector<std::string>& xFields, std::vector<std::string>& yFields);
        const std::map<std::string, int>& getColumnNames() const;
        const std::vector<std::vector<std::string>>& getDataLines() const;

        PointCsvFile(bool polygon);
        ~PointCsvFile() = default;
    };

} // namespace Ilvo
} // namespace Utils
} // namespace File

