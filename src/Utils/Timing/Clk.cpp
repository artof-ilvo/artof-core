#include <Utils/Timing/Clk.h>
#include <sstream>   

using namespace Ilvo::Utils::Timing;

using namespace std;
using namespace chrono_literals;
using namespace chrono;
using chrono::duration_cast;
using chrono::milliseconds;
using chrono::system_clock;

Clk::Clk(milliseconds interval) : 
    timerOn(false),
    interval(interval), 
    timerInterval(20ms) 
{}

Clk::Clk() : 
    timerOn(false), 
    interval(100ms)
{}

void Clk::start() {
    startTime = system_clock::now();
}

double Clk::poll() {
    auto endTime = system_clock::now();
    duration<double, milli> elapsed {endTime - startTime};
    return elapsed.count();
}

void Clk::stop() {
    this_thread::sleep_until(startTime+interval);
}

void Clk::startTimer(milliseconds timerInterval) {
    this->timerInterval = timerInterval;
    this->timerStartTime = system_clock::now();
    timerOn = true;
}

bool Clk::checkTimerBusy() {
    if (timerOn) {
        auto endTime = system_clock::now();
        // duration<double, milli> elapsed {endTime - timerStartTime};
        auto elapsed = duration_cast<milliseconds>(endTime - timerStartTime);
        // LoggerStream::getInstance() << DEBUG << elapsed.count() << ", " << timerInterval.count() << endl;
        if (elapsed < timerInterval) {
            return true;
        } else {
            timerOn = false;
        }
    } 
    return false;
}

bool Clk::checkTimerExpired() {
    if (timerOn) {
        auto endTime = system_clock::now();
        // duration<double, milli> elapsed {endTime - timerStartTime};
        auto elapsed = duration_cast<milliseconds>(endTime - timerStartTime);
        return (elapsed > timerInterval);
    } 
    return false;
}

int Clk::getIntervalMs()
{
    return interval.count();
}