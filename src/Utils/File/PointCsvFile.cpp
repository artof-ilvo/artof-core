#include <Utils/File/PointCsvFile.h>
#include <Utils/File/File.h>
#include <Exceptions/FileExceptions.hpp>
#include <boost/algorithm/string.hpp>

#include <filesystem>
#include <sstream>
#include <fstream>

using namespace Ilvo::Utils::File;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Exception;

using namespace std;
using namespace chrono_literals;
using namespace boost::algorithm;
using namespace std::filesystem;

using chrono::duration_cast;

PointCsvFile::PointCsvFile(bool polygon) : PointData(polygon)
{
}

void PointCsvFile::init(const string& filename, vector<string>& xFields, vector<string>& yFields)
{
    csv_file = filename;
    // Check if filename is of .csv format
    string delimiter = ".";

    // Check file extension
    size_t pos = 0;
    string token;
    pos = filename.find(delimiter);
    string extention = filename.substr(pos, filename.length());

    if (extention != ".csv")
    {
        throw WrongFileExtensionException(extention, ".csv");
    }

    // Check if .csv file exists
    if ( !exists( csv_file ) )
    {
        throw PathNotFoundException( csv_file );
    }

    // clear array
    series.clear(); 

    // Read series
    this->readPointFile(xFields, yFields);

}

void PointCsvFile::readPointFile(vector<string>& xFields, vector<string>& yFields) {
    // Create an input filestream
    ifstream myFile( csv_file );
    // Make xFields and yFields lower case
    for (string& xf : xFields) {
        to_lower(xf);
    }
    for (string& yf : yFields) {
        to_lower(yf);
    }

    // Make sure the file is open
    if(!myFile.is_open()) {
        throw PathNotFoundException( csv_file );
    }

    // Helper vars
    string line, colname;
    int xColIdx = -1;
    int yColIdx = -1;
    int colIdx;

    // Read the column names
    if(myFile.good())
    {
        // Extract the first line in the file
        getline(myFile, line);
        removeCharacters(line, CHARS_END_OF_LINE);

        // Create a stringstream from line
        stringstream ss(line);

        // Extract each column name
        colIdx = 0;
        while(getline(ss, colname, ',')){
            string colname_orig = colname;
            to_lower(colname);
            if (count(xFields.begin(), xFields.end(), colname)) {
                xColIdx = colIdx;
            } else if (count(yFields.begin(), yFields.end(), colname)) {
                yColIdx = colIdx;
            } 
            column_names.insert(make_pair(colname_orig, colIdx));
            colIdx++;
        }
    }

    // check if columns occured
    if (! (xColIdx >= 0 && yColIdx >= 0)) {
        throw CsvColumnsNotFoundException(xFields, yFields);
    }

    // Read data, line by line
    this->series.push_back(vector<PointPtr>()); // add new pointvector
    while(getline(myFile, line))
    {
        removeCharacters(line, CHARS_END_OF_LINE);
        // don't process when no data
        if (line.empty()) break;

        // helper var -> keeps the point
        vector<double> point;
        // Create a stringstream of the current line
        stringstream ss(line);
             
        // Extract each integer
        double x = -1.0;
        double y = -1.0;
        colIdx = 0;
        string colVal;
        vector<string> data_line = {};
        while(getline(ss, colVal, ',')){
            removeCharacters(colVal, CHARS_BRACKETS);

            if (xColIdx == colIdx) {
                x = stod(colVal);
            } else if (yColIdx == colIdx) {
                y = stod(colVal);
            }
            data_line.push_back(colVal);
            colIdx++;
        }
        
        data_lines.push_back(data_line);
        series[0].push_back(make_shared<Point>(x, y));
    }
    // if polygon push last point to the array
    if (polygon && this->getPoints(0).size() > 0) {
        vector<PointPtr> p_first_series = this->getPoints(0);
        this->series[0].push_back(make_shared<Point>(p_first_series[0]->x(), p_first_series[0]->y()));
    }
    // Close file
    myFile.close();
}

const map<string, int>& PointCsvFile::getColumnNames() const
{
    return column_names;
}
const vector<vector<string>>& PointCsvFile::getDataLines() const
{
    return data_lines;
}
