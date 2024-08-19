/**
 * @file Section.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Section settings loaded from the implement json file
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <memory>
#include <Utils/Settings/State.h>
#include <ThirdParty/json.hpp>
#include <Utils/Geometry/Polygon.h>
#include <Utils/Geometry/Point.h>

namespace Ilvo {
namespace Utils {
namespace Settings {

class Section : public StateFull
{
private:
    std::vector<Geometry::PointPtr> activation_geometry_points;
    std::vector<Geometry::PointPtr> activation_geometry_points_section_cs;
    TransformMatrix parallel_transform; 
    // no json settings variable
    double parallel_angle;
    Geometry::Polygon p;
public:
    std::string id;
    double width;
    double up;
    double down;
    bool active;
    double link_length;
    int repeats;
    double offset;
    
    Section(std::string id, double width, 
            double up=0.10, double down=0.10, 
            double link_length=0.0, 
            TransformMatrix parallel_transform=TransformMatrix());
    Section(Section& section);
    Section(nlohmann::json j);
    ~Section() = default;

    double getLinkLength();
    void setParallelAngle(double angle);
    double getParallelAngle();
    void setActive(bool active);
    bool getActive();

    void clearActivationGeometry();
    void addActivationGeometry(Geometry::PointPtr g);
    void setActivationGeometry(Geometry::PolygonPtr g);

    Eigen::Affine3d getParallelTransform();
    const Geometry::Polygon& getPolygon() const;
    void reset();
    void updatePolygon();

    nlohmann::json prepareJson() const;

    nlohmann::json visualizeJson(int zone=-1) const;
};

typedef std::shared_ptr<Section> SectionPtr;


} // namespace
} // namespace
} // namespace