#pragma once

#include <thread>
#include <atomic>
#include <deque>
#include <mutex>
#include <condition_variable>

#include <Utils/Nmea/Nmea.h>
#include <Utils/Nmea/NmeaMessagePack.h>

namespace Ilvo {
namespace Utils {
namespace Peripheral {

    class Peripheral
    {
    protected:
        int fd;
        static const int MAX_BUF_SIZE = 1;
        std::shared_ptr<Nmea::NmeaLine> nmeaLine;
        // variable logic
        Nmea::NmeaMessagePack nmeaMessagePack;
        bool lineUpdated = false;

        std::thread t;
        std::atomic<bool> RUNNING = ATOMIC_VAR_INIT(true); // syncing the threads

        std::mutex m;
        std::condition_variable reader_action;
    public:
        bool loaded = false;
    public:
        Peripheral() = default;
        ~Peripheral() = default;

        virtual bool openFd() = 0;
        virtual bool closeFd() = 0;
        virtual bool readNmeaLine() = 0;

        int getFd();
        void waitForOpenFd();
        
        std::shared_ptr<Nmea::NmeaMessagePack> getNmeaLines();
        void addNmeaLine();
        
        virtual void init() = 0;
        virtual void run() = 0;

        void start();
        void stop();
    };

} // Peripheral
} // Utils
} // Ilvo
