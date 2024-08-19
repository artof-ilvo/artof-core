#include <Utils/Timing/Timing.h>
#include <Utils/Logging/LoggerStream.h>
#include <string>
#include <iostream>
#include <thread>

using namespace Ilvo::Utils::Timing;
using namespace Ilvo::Utils::Logging;

using namespace std;
using namespace chrono;
using namespace chrono_literals;

void Ilvo::Utils::Timing::waitForNetworkConnection(string ip, seconds interval)
{
    bool internetconnection = false;
    string cmd = "ping -c 1 " + ip + " > /dev/null";
    while (!internetconnection) {
        internetconnection = (system(cmd.c_str()) == 0);
        if (internetconnection) {
            LoggerStream::getInstance() << DEBUG << "Got network connection access to " << ip;
        } else {
            LoggerStream::getInstance() << DEBUG << "No network access to " << ip << ", waiting for 10 secs ...";
            this_thread::sleep_for(10s);
            waitForNetworkConnection(ip, interval);
        }
    }
}

string Ilvo::Utils::Timing::toISO8601Format(system_clock::time_point& t) {
    time_t timet = chrono::system_clock::to_time_t(t);
    string ret = (stringstream() << std::put_time(localtime(&timet), "%FT%T")).str();
    return ret;
}

void Ilvo::Utils::Timing::tic(int mode) 
{
    static std::chrono::_V2::system_clock::time_point t_start;
    
    if (mode==0)
        t_start = std::chrono::high_resolution_clock::now();
    else {
        auto t_end = std::chrono::high_resolution_clock::now();
        LoggerStream::getInstance() << DEBUG << "Elapsed time is " << (t_end-t_start).count()*1E-9 << " seconds\n";
    }
}

void Ilvo::Utils::Timing::toc() 
{ 
    Ilvo::Utils::Timing::tic(1); 
}

double Ilvo::Utils::Timing::ticHz(int mode)
{
    static std::chrono::_V2::system_clock::time_point t_start;
    
    if (mode==0) {
        t_start = std::chrono::high_resolution_clock::now();
        return 0.0;
    } else {
        auto t_end = std::chrono::high_resolution_clock::now();
        return 1/((t_end-t_start).count()*1e-9);
    }
}

double Ilvo::Utils::Timing::tocHz()
{
    return Ilvo::Utils::Timing::ticHz(1);
}