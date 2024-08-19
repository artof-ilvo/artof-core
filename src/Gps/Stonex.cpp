#include <Gps/Stonex.h>
#include <Utils/Logging/LoggerStream.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Peripheral;
using namespace Ilvo::Utils::Logging;

using namespace std;

Stonex::Stonex(string host, int port) : 
    Socket(host, port) 
{}

Stonex::~Stonex() {
    stop();
}

void Stonex::init() 
{}

void Stonex::run() {
    while (RUNNING.load()) {
        if (!readNmeaLine()) {
            LoggerStream::getInstance() << INFO << "Recover Fd connection:"; // TODO check if recover is necessary/good idea
            // waitForOpenFd();
        } else {
            loaded = true;
        }
    }
    closeFd();
}
