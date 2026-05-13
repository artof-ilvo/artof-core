#pragma once

#include <Utils/File/PointData.h>
#include <ThirdParty/json.hpp>
#include <string>

namespace Ilvo {
namespace Utils {
namespace File {

    class PointGeoJsonFile : public PointData
    {
    private:
        int utmZone;
        void addSegment(const nlohmann::json& feature);

    public:
        PointGeoJsonFile(int utmZone);
        ~PointGeoJsonFile() = default;

        void init(const std::string& filename);
    };

} 
} 
} 
