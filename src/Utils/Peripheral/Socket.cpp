#include <sstream>
#include <chrono>
#include <netdb.h>

#include <sys/socket.h> 
#include <arpa/inet.h> 


#include <Utils/Peripheral/Socket.h>
#include <Utils/Nmea/Nmea.h>
#include <Utils/Logging/LoggerStream.h>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

using namespace Ilvo::Utils::Nmea;
using namespace Ilvo::Utils::Peripheral;
using namespace Ilvo::Utils::Logging;
using namespace boost::asio;

using namespace std;

Socket::Socket(string host, int port) : 
    Peripheral(), 
    host(host), 
    port(port)
{}

bool Socket::openFd() {
    struct sockaddr_in serv_addr; 
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        LoggerStream::getInstance() << DEBUG << "Socket creation failed.";
        return false; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(this->port); 
       
    // retreive sin_addr of serv_addr
    boost::system::error_code ec;
    ip::address::from_string(host, ec);
    if (ec) { // handle address as hostname
        struct hostent *he;
        if(!(he=gethostbyname(host.c_str()))) {
            LoggerStream::getInstance() << WARN << "Gethostbyname failed.";
            return false;
        }
        serv_addr.sin_addr = *((struct in_addr *)he->h_addr);
        memset(&(serv_addr.sin_zero), '\0', 8);
    } else { // handle address as host address
        // Convert IPv4 and IPv6 addresses from text to binary form 
        if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr)<=0) { 
            LoggerStream::getInstance() << WARN << "Invalid address/ Address not supported."; 
            return false; 
        } 
    }

    // Set connection timeout to 10 seconds
    struct timeval tv;
    tv.tv_sec = 10; // Set timeout to 10 seconds
    tv.tv_usec = 0; // Microseconds can be set for finer control (optional)

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        LoggerStream::getInstance() << WARN << "Setting timeout interval using setsockopt failed.";
        return false;
    }

    // Connect
    LoggerStream::getInstance() << INFO << "Waiting for socket connect to \"" << host << ":" << port << "\" ...";
    if (::connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        LoggerStream::getInstance() << WARN << "Connection error: " << strerror(errno);

        if (errno == ETIMEDOUT)  LoggerStream::getInstance() << WARN << "Connection timed:";
        else LoggerStream::getInstance() << WARN << "Connection failed with error: " << strerror(errno);

        LoggerStream::getInstance() << WARN << "Connection to \"" << host <<  ":" << port << "\" failed!";
        LoggerStream::getInstance() << WARN << "Check your network connection! Are you on the ILVO network? Can you ping to google.com? Is there a blocking firewall!";

        return false; 
    } 
    LoggerStream::getInstance() << INFO << "Connected to \"" << host << "\" successfully!";

    return true; 
}

bool Socket::closeFd() {
    ::close(fd);
    return true;
}

bool Socket::readNmeaLine() {
    vector<char> line;

    // Read bytes. The behaviour of read() (e.g. does it block?,
    // how long does it block for?) depends on the configuration
    // settings above, specifically VMIN and VTIME
    char readchar;
    int num_bytes = ::read(fd, &readchar, sizeof(char));

    // read sentence
    while (readchar != '$') {
        num_bytes = ::read(fd, &readchar, sizeof(char));
    }
    line.push_back('$');

    // read the rest of the line in the line vector
    num_bytes = ::read(fd, &readchar, sizeof(char));
    while (readchar != '\r') { 
        if (num_bytes < 0) {
            LoggerStream::getInstance() << ERROR << "Error: reading FD " << fd << " - bytes " << num_bytes;
            return false;
        }
        line.push_back(readchar);
        num_bytes = ::read(fd, &readchar, sizeof(char));
    }

    // create NmeaLine
    nmeaLine = make_shared<NmeaLine>(line);
    // add line to queue
    addNmeaLine();

    return true;
}