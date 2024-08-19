/**
 * @file Implement.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Implement settings load from the implement json files
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Settings/Hitch.h>
#include <Utils/Settings/Section.h>

#include <string>

namespace Ilvo {
namespace Utils {
namespace Settings {

class Implement
{
private:
    std::string name;
    bool onTaskmap;
    std::vector<std::string> types;
    std::vector<std::shared_ptr<Section>> sections;
public:
    Implement() = default;
    Implement(double defaultWidth);
    Implement(std::string name, double defaultWidth);
    Implement(nlohmann::json j);
    ~Implement() = default;

    bool worksOnTaskmap() const;
    const std::string& getName() const;
    void loadJson(nlohmann::json j_impl);
    bool empty() const;

    void resetSections();
    std::vector<std::shared_ptr<Section>>& getSections();

    nlohmann::json toStateFullJson();

    nlohmann::json visualizeJson(int zone=-1) const;
};

} // namespace
} // namespace
} // namespace
