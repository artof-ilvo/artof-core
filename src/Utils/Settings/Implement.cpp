#include <Utils/Settings/Implement.h>
#include <Utils/Settings/Section.h>
#include <Utils/Logging/LoggerStream.h>
#include <ThirdParty/json.hpp>
#include <boost/filesystem.hpp>
#include <Exceptions/FileExceptions.hpp>
#include <Exceptions/RobotExceptions.hpp>
#include <Utils/Geometry/Polygon.h>

#include <iostream>
#include <fstream>

using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Exception;
using namespace boost::filesystem;

using namespace std;
using namespace nlohmann;

Implement::Implement(double defaultWidth) :
    name("default")
{
    sections.push_back(make_shared<Section>("0", defaultWidth));
}

Implement::Implement(string name, double defaultWidth) :
    name(name)
{
    if (name.empty() || name.compare("None") == 0 || name.compare("") == 0) {
        LoggerStream::getInstance() << INFO << "Implement name is empty.";
        Implement(defaultWidth);
        return;
    } 
    
    string implFilePath = string(getenv("ILVO_PATH")) + "/implement/" + name + ".json";
    if (!exists(implFilePath)) {
        LoggerStream::getInstance() << INFO << "The corresponding implement file \"" << implFilePath << "\" does not exist.";
        Implement(defaultWidth);
        return;
    }
    
    json j_impl;
    try {
        j_impl = json::parse(std::ifstream(implFilePath));
    } catch(json::exception& e) {
        LoggerStream::getInstance() << ERROR << "Implement parse error for \"" << name << "\", " << e.what();
    }
    
    // initialize component
    loadJson(j_impl);
    LoggerStream::getInstance() << INFO << "Implement \"" << name << "\" loaded successfully.";
}

Implement::Implement(nlohmann::json j)
{
    loadJson(j);
}

void Implement::loadJson(json j_impl)
{
    if (j_impl.contains("name")) name = j_impl["name"].get<string>();
    else throw SettingsParamNotFoundException("Implement", "name");

    if (j_impl.contains("on_taskmap")) onTaskmap = j_impl["on_taskmap"];
    else onTaskmap = true;

    if (j_impl.contains("types")) {
        for (json j_type: j_impl["types"]) {
            types.push_back(j_type);
        }
    } 
    else throw SettingsParamNotFoundException("Implement", "types");

    if (j_impl.contains("sections")) {
        for (json j_section: j_impl["sections"]) {
            Section section = Section(j_section);
            for (int i=0; i < section.repeats; i++) {
                if (section.repeats > 1) {
                    Section newSection(section);
                    newSection.id = section.id + std::to_string(i);
                    newSection.getRefTransform()(0, 3) += i * section.offset;
                    sections.push_back(make_shared<Section>(newSection));
                } else {
                    sections.push_back(make_shared<Section>(section));
                }
            } 
        }
    } else {
        throw SettingsParamNotFoundException("Task", "type");
    }
}

const string& Implement::getName() const
{
    return name;
}

bool Implement::empty() const
{
    return name.empty() || name == "None" || name == "default";
}

bool Implement::worksOnTaskmap() const
{
    return onTaskmap;
}

vector<shared_ptr<Section>>& Implement::getSections()
{
    return sections;
}

json Implement::toStateFullJson()
{
    json j_impl;
    j_impl["name"] = name;
    j_impl["on_taskmap"] = onTaskmap;
    j_impl["types"] = json::array();
    for (string& type: types) {
        j_impl["types"].push_back(type);
    }
    j_impl["sections"] = json::array();
    for (auto section: sections) {
        j_impl["sections"].push_back(section->toStateFullJson());
    } 
    return j_impl;
}

json Implement::visualizeJson(int zone) const
{
    json j = json();
    j["name"] = name;
    j["sections"] = json::array();
    for (auto section: sections) {
        j["sections"].push_back(section->visualizeJson(zone));
    } 
    return j;
}

void Implement::resetSections()
{
    for (auto section: sections) {
        section->reset();
    }
}