#include <Gps/Simplertk3b.h>
#include <Utils/Logging/LoggerStream.h>
#include <mutex>
#include <chrono>

using namespace std;
using namespace Ilvo::Core;
using namespace Ilvo::Utils::Peripheral;
using namespace Ilvo::Utils::Logging;

Simplertk3b::Simplertk3b(string serialportname, NtripCredentials& credentials) : 
    Serial(serialportname),
    credentials(credentials)
{
}


Simplertk3b::~Simplertk3b() {
    if (ntrip != nullptr) ntrip->stop();
    stop();
}

void Simplertk3b::init() {
    // wait for ntrip
    ntrip = make_unique<Ntrip>(&credentials, this, 10);
    ntrip->start();
}

void Simplertk3b::run() {
    while (RUNNING.load()) {
        if (!readNmeaLine()) {
            LoggerStream::getInstance() << INFO << "Recover Fd connection:";  // TODO check if recover is necessary/good idea
            // serial.waitForOpenFd();
        } else {
            loaded = true;
            ntrip->sendGga();
        }
    }
    closeFd();
}