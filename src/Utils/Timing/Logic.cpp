#include <Utils/Timing/Logic.h>


using namespace Ilvo::Utils::Timing;

using namespace std;
using namespace chrono;

EdgeDetector::EdgeDetector() : 
    previousValue(false), rising(false), falling(false)
{}

void EdgeDetector::detect(bool newValue) 
{
    rising = !previousValue && newValue; 
    falling = previousValue && !newValue;
    previousValue = newValue; 
}

bool EdgeDetector::getValue()
{
    return previousValue;
}

SinglePulseGenerator::SinglePulseGenerator() : timerActivated(false) {}

bool SinglePulseGenerator::generatePulse(milliseconds timerInterval_ms)
{
    if (!timerActivated) { 
        clk.startTimer(timerInterval_ms);
        timerActivated = true;
    } 

    if (clk.checkTimerBusy()) {
        return true;
    } else {
        timerActivated = false;
    }
    
    return false;
}

PulseGenerator::PulseGenerator(milliseconds interval_ms) : 
    timerInterval_ms(interval_ms), timerActivated(false), p(false)
{}

PulseGenerator::PulseGenerator(uint interval_ms) : 
    timerInterval_ms(milliseconds(interval_ms)), timerActivated(false), p(false)
{}

bool PulseGenerator::generatePulse()
{
    if (!timerActivated) { 
        clk.startTimer(timerInterval_ms);
        timerActivated = true;
    } 

    if (!clk.checkTimerBusy()) {
        p = !p;
        // LoggerStream::getInstance() << DEBUG << std::boolalpha;
        // LoggerStream::getInstance() << DEBUG << "HEARTBEAT: " << p << endl;;
        clk.startTimer(timerInterval_ms);
    }
    return p;
}

void PulseGenerator::setInterval(milliseconds interval_ms)
{
    timerInterval_ms = interval_ms;
}

void PulseGenerator::setInterval(uint interval_ms)
{
    timerInterval_ms = milliseconds(interval_ms);
}

uint PulseGenerator::getInterval()
{
    return timerInterval_ms.count();
}

bool PulseGenerator::getValue() {
    return p;
}

TimeEdgeDetector::TimeEdgeDetector(milliseconds detectInterval_ms) : 
    detectInterval_ms(detectInterval_ms), previousValue(false), isExpired(false)
{
    clk.startTimer(detectInterval_ms);
}

bool TimeEdgeDetector::expired(bool newValue)
{
    if (newValue != previousValue) {
        clk.startTimer(detectInterval_ms);
        previousValue = newValue;
        isExpired = false;
        // std::cout << "Time started again - previousValue: " << previousValue << ", newValue: " << newValue << ", detectInterval_ms: " << detectInterval_ms.count() << ", elapsed: " << clk.poll() << endl;
    }

    if(clk.checkTimerExpired() && !isExpired) {
        isExpired = true;
        return true;
    };

    return false;
}