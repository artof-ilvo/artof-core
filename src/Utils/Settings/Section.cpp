#include <Utils/Settings/Section.h>
#include <Utils/Geometry/Transform.h>
#include <Exceptions/RobotExceptions.hpp>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Exception;
using namespace Eigen;
using namespace nlohmann;
using namespace std;

Section::Section(string id, double width, double up, double down, double link_length, TransformMatrix parallel_transform) : 
    id(id), width(width), active(false), up(up), down(down), link_length(link_length), parallel_transform(parallel_transform), parallel_angle(0.0)
{}

Section::Section(Section& section) : 
    StateFull(section.getRefTransform()), id(section.id), width(section.width), up(section.up), down(section.down), link_length(section.link_length), parallel_transform(section.parallel_transform), parallel_angle(section.parallel_angle)
{}

Section::Section(json j) :
    StateFull(j["transform"])
{
    if (j.contains("id")) id = j["id"].get<string>(); else id = "";

    if (j.contains("width")) width = j["width"];
    else throw SettingsParamNotFoundException("section", "width");

    if (j.contains("up")) up = j["up"];
    else throw SettingsParamNotFoundException("section", "up");

    if (j.contains("down")) down = j["down"];
    else throw SettingsParamNotFoundException("section", "down");

    if (j.contains("active")) active = j["active"];
    else active = false;

    // No obligatory fields
    if (j.contains("repeats")) {
        repeats = j["repeats"]; 
        if (repeats < 1) repeats = 1;
    } else {
        repeats = 1;
    };
    if (j.contains("offset")) offset = j["offset"]; else offset = 0.0;
    if (j.contains("link_length")) link_length = j["link_length"];
    if (j.contains("parallel_transform")) parallel_transform = TransformMatrix(j["parallel_transform"]);
}

void Section::updatePolygon()
{    
    // make sure polygon is closed!
    p.update(getState().asAffine(), width, up, -down);
}

const Polygon& Section::getPolygon() const
{
    return p;
}

Eigen::Affine3d Section::getParallelTransform()
{
    return calculateHingeTransform(link_length, parallel_angle) * parallel_transform * getRefTransform();
}

double Section::getLinkLength()
{
    return link_length;
}

void Section::setParallelAngle(double angle)
{
    parallel_angle = angle;
}

double Section::getParallelAngle()
{
    return parallel_angle;
}

void Section::setActive(bool active)
{
    this->active = active;
}

bool Section::getActive()
{
    return active;
}

void Section::clearActivationGeometry()
{
    activation_geometry_points.clear();
    activation_geometry_points_section_cs.clear();
}

void Section::addActivationGeometry(Geometry::PointPtr p)
{
    Affine3d sectionAffine = getState().asAffine();
    sectionAffine(2, 3) = 0.0; // zero ground level
    Vector4d point_world_cs = {p->x(), p->y(), 0.0, 1};
    Vector4d p_section_cs = sectionAffine.inverse() * point_world_cs;
    
    activation_geometry_points_section_cs.push_back(make_shared<Point>(p_section_cs(0), p_section_cs(1)));
    activation_geometry_points.push_back(p);
}

void Section::setActivationGeometry(PolygonPtr g) 
{
    for (auto p: g->geometry().outer()) {
        addActivationGeometry(make_shared<Point>(p.x(), p.y()));
    }
}

void Section::reset()
{
    active = false;
}

json Section::prepareJson() const
{
    json j;
    j["id"] = id;
    j["width"] = width;
    j["up"] = up;
    j["down"] = down;
    j["parallel_angle"] = parallel_angle;
    j["active"] = active;
    j["activation_geometry"] = json::array();
    for (auto p: activation_geometry_points) {
        j["activation_geometry"].push_back(p->toJson());
    }
    j["activation_geometry_section_cs"] = json::array();
    for (auto p: activation_geometry_points_section_cs) {
        j["activation_geometry_section_cs"].push_back(p->toJson());
    }
    return j;
}


nlohmann::json Section::visualizeJson(int zone) const
{
    vector<vector<double>> robotLatLng;
    vector<vector<double>> robotXY;
    getPolygon().contour(robotLatLng, robotXY, zone);
    json j = json();
    j["latlng"] = robotLatLng;
    j["xy"] = robotXY;
    j["id"] = id;
    j["active"] = active;

    return j;
}