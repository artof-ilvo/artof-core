#include <Utils/Peripheral/Serial.h>
#include <Utils/Nmea/Nmea.h>
#include <Utils/Logging/LoggerStream.h>

#include <cstddef> 
#include <iostream>
#include <thread>

using namespace Ilvo::Utils::Peripheral;
using namespace Ilvo::Utils::Nmea;
using namespace Ilvo::Utils::Logging;

using namespace std;

Serial::Serial(std::string portname) : 
    Peripheral(),
    portname(portname),
    ggaLine("$GPGGA,125128.60,5058.9589820,N,00346.6700139,E,1,15,1.4,26.6423,M,47.2596,M,,*5E\r\n")  // default gga line to start the ntrip server
{}

std::string& Serial::getGgaLine() { 
    return ggaLine; 
}

// abstract functions
bool Serial::openFd() {
    struct termios tty;

    // Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
    int serial_port = open(portname.c_str(), O_RDWR);
    if (serial_port == -1) {
        LoggerStream::getInstance() << WARN << "Error " << errno << " from open: " << strerror(errno);
        return false;
    }
    memset(&tty, 0, sizeof tty);

    // Read in existing settings, and handle any error
    if(tcgetattr(serial_port, &tty) != 0) {
        LoggerStream::getInstance() << WARN << "Error " << errno << " from tcgetattr: " << strerror(errno);
        return false;
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B921600); // instead of B38400
    cfsetospeed(&tty, B921600); // instead of B38400

    // Save tty settings, also checking for error
    LoggerStream::getInstance() << INFO << "Connecting to serial GPS device...";
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        LoggerStream::getInstance() << WARN << "Error " << errno << " from tcsetattr: " << strerror(errno);
        LoggerStream::getInstance() << WARN << "Connecting to serial GPS failed!";
        return false;
    }
    LoggerStream::getInstance() << INFO << "Connecting to serial GPS successfull!";

    fd = serial_port;
    return true;
}

// Close the serial port
bool Serial::closeFd() {
    close(fd);
    return true;
}

bool Serial::readNmeaLine()
{
    std::vector<char> line;

    // Read bytes. The behaviour of read() (e.g. does it block?,
    // how long does it block for?) depends on the configuration
    // settings above, specifically VMIN and VTIME
    char readchar;
    int num_bytes = read(fd, &readchar, sizeof(char));

    // read sentence
    while (readchar != '$') {
        num_bytes = read(fd, &readchar, sizeof(char));
    }
    line.push_back('$');

    // rad the rest of the line in vector
    num_bytes = read(fd, &readchar, sizeof(char));
    while (readchar != '\r' && readchar != '\n') { 
        if (num_bytes < 0) {
            LoggerStream::getInstance() << DEBUG << "Error: reading fd " << fd << " - bytes " << num_bytes;
            return false;
        }
        line.push_back(readchar);
        num_bytes = read(fd, &readchar, sizeof(char));
    }

    // create NmeaLine
    nmeaLine = make_shared<NmeaLine>(line);
    // update gga string, make sure it's not empty 
    if (nmeaLine->ok()) {
        // make sure it's a GGA
        if (nmeaLine->getType() == NmeaMessageType::GGA) {
            // Make sure fix value is either fix (4), float (5) or dgps (2)
            vector<int> permittedFixValues = {4, 5, 2};
            int currentFix = nmeaLine->getValue<int>("fix");
            if (std::find(permittedFixValues.begin(), permittedFixValues.end(), currentFix) == permittedFixValues.end()) {
                ggaLine = nmeaLine->str() + "\r\n";   
            }
        }
    }
    // add line to queue
    addNmeaLine();

    return true;
}

void Serial::writeRtcm(char* msg, int size) {
    if (fd != -1) {
        int numbytes = 0;
        if ((numbytes = write(fd, msg, size)) != size) {
            LoggerStream::getInstance() << ERROR << "Writing to fd " << fd <<" failed!";
            LoggerStream::getInstance() << ERROR << "errno: " << (int) errno;
        } else {
            // LoggerStream::getInstance() << DEBUG << numbytes << " bytes written to fd " << fd;
        }
    } else {
        LoggerStream::getInstance() << DEBUG << "Writing to fd " << (int) fd << " failed.";
    }
}

void Serial::writeLine(std::string& str) {
    if (fd != -1) {
        int numbytes = 0;
        if ((numbytes = write(fd, str.data(), str.size())) != str.size()) {
            LoggerStream::getInstance() << ERROR << "Writing to fd " << fd <<" failed!";
            LoggerStream::getInstance() << ERROR << "errno: " << (int) errno;
        } else {
            // LoggerStream::getInstance() << DEBUG << numbytes << " bytes written to fd " << fd;
        }
    }
}