#pragma once

// C library headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <string>

#include <Utils/Peripheral/Peripheral.h>

namespace Ilvo {
namespace Utils {
namespace Peripheral {
    
    /**
     * @brief Socket class: reads the nmea messages from the gps module.
     * 
     */
    class Socket : public Peripheral
    {   
    private:
        std::string host;
        int port;
    public:
        Socket() = default;
        Socket(std::string host, int port);
        ~Socket() = default;

        bool openFd() override;
        bool closeFd() override;
        bool readNmeaLine() override;

        virtual void init() = 0;
        virtual void run() = 0;

    };

} // Peripheral
} // Utils
} // Ilvo