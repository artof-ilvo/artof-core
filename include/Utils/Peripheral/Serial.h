/**
 * @file Serial.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief 
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO) 
 * 
 */

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

    class Serial : public Peripheral
    {
    private:
        int serialfd;

        std::string portname;
        std::string ggaLine;
    public:
        Serial(std::string portname);

        bool openFd() override;
        bool closeFd() override;

        void writeLine(std::string&);

        bool readNmeaLine() override;
        void writeRtcm(char* msg, int size);
        std::string& getGgaLine();

        virtual void init() = 0;
        virtual void run() = 0;
    };

} // Peripheral
} // Utils
} // Ilvo