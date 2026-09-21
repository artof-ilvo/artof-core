#include <Utils/File/PointData.h>

using namespace Ilvo::Utils::File;
using namespace Ilvo::Utils::Geometry;

using namespace std;

PointData::PointData(bool polygon) :
    polygon(polygon), series(vector<vector<PointPtr>>())
{}

const vector<vector<PointPtr>>& PointData::getAllPoints()
{
    return this->series;
}

vector<PointPtr> PointData::getAllPointsFlat()
{
    vector<PointPtr> flatPoints;
    for (const auto& serie : this->series) {
        flatPoints.insert(flatPoints.end(), serie.begin(), serie.end());
    }
    return flatPoints;
}

int PointData::getNumSeries()
{
    return int(this->series.size());
}

bool PointData::hasMultiple() 
{
    return this->series.size() > 1;
}

vector<PointPtr>& PointData::getPoints(uint i)
{
    return this->series[i];
}

vector<ShapeFieldDataPtr>& PointData::getFields(uint i)
{
    return this->metadata[i];
}

int PointData::getNumPoints(uint serie)
{
    return this->series[serie].size();
}

int PointData::getNumFields(uint serie)
{
    return int(this->metadata[serie].size());
}

bool PointData::isPolygon(uint i)
{
    vector<PointPtr> p_first_series = this->getPoints(i);
    return *p_first_series[0] == *p_first_series[p_first_series.size()-1];
}
