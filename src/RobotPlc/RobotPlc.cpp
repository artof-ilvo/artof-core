#include <Utils/Timing/Clk.h>
#include <chrono>
#include <thread>
#include <Utils/Redis/PlcVariableManager.h>
#include <Utils/Logging/LoggerStream.h>
#include <Exceptions/FileExceptions.hpp>

using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Logging;
using namespace Ilvo::Exception;

using namespace std;
using namespace chrono_literals;

int main() {    
    // First check if ILVO_PATH environment variable is set
    if (getenv("ILVO_PATH") == NULL) { 
        throw EnvVariableNotFoundException("$ILVO_PATH");
    }

    string procName = "ilvo-robot-plc";
    LoggerStream::createInstance(procName);
    LoggerStream::getInstance() << INFO << "Robot Plc Logger started";
    PlcVariableManager robotPlc(procName);
    robotPlc.run();
    return 0;    
}
