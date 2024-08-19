#include <Utils/Docker/DockerRegistry.h>
#include <ThirdParty/base64/base64.h>

using namespace Ilvo::Utils::Docker;
using namespace nlohmann;


DockerRegistry::DockerRegistry(json registry) :
    registry(registry)
{
    parse(registry);
}

void DockerRegistry::parse(json registry) 
{
    this->registry = registry;
    if (registry.contains("username")) {
        authConfigHeader["X-Registry-Auth"] = base64_encode(registry.dump());
    }
}

json& DockerRegistry::asHeader() {
    return authConfigHeader;  
}

nlohmann::json DockerRegistry::toJson() {
    return registry;
}