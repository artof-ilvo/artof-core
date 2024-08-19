#include <Utils/Peripheral/Peripheral.h>
#include <Utils/Timing/Timing.h>
#include <Utils/Logging/LoggerStream.h>
#include <chrono>
#include <iostream>

using namespace Ilvo::Utils::Nmea;
using namespace Ilvo::Utils::Timing;
using namespace Ilvo::Utils::Peripheral;
using namespace Ilvo::Utils::Logging;

using namespace std;

shared_ptr<NmeaMessagePack> Peripheral::getNmeaLines() { 
    // double hz = tocHz();
    // flush(cout);
    auto lock = unique_lock<mutex>(m); // unique_lock can be unlocked, lock_guard can not
    reader_action.wait(lock, [this] { return nmeaMessagePack.isUpdated(); });
    nmeaMessagePack.reset();
    // ticHz();
    // LoggerStream::getInstance() << DEBUG << "\rFrequency: " << hz << " Hz.";

    return make_shared<NmeaMessagePack>(NmeaMessagePack(nmeaMessagePack));
}

void Peripheral::addNmeaLine() {
    // Adding line to queue is critical
    auto lock = unique_lock<mutex>(m); 
    // START CRITICAL SECTION
    // using varaible
    nmeaMessagePack.addNmeaLine(nmeaLine);
    // END CRITICAL SECTION
    lock.unlock();
    reader_action.notify_one(); // wakes up reader  
}

int Peripheral::getFd() { 
    return fd; 
}

void Peripheral::waitForOpenFd() {
    LoggerStream::getInstance() << INFO << "Making connection...";
    while(!openFd()) {
        LoggerStream::getInstance() << WARN << "Connection failed. Waiting for 10 secs to retry connection.";
        this_thread::sleep_for(10s);
    }
    LoggerStream::getInstance() << INFO << "Connected successfully!";
}

void Peripheral::start() {
    waitForOpenFd();
    init();
    t = thread(&Peripheral::run, this);
}

void Peripheral::stop() {
    RUNNING.store(false); // make threads stop
    if (t.joinable()) t.join(); // break thread down when ready
    closeFd();
}