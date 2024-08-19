/**
 * @file Ntrip.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Ntrip client class, maintaining the NTRIP connection with the NTRIP server.
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#pragma once

#include <string>
#include <chrono>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */

#include <Utils/Peripheral/Socket.h>
#include <Utils/Peripheral/Serial.h>

namespace Ilvo {
namespace Core {

/* The string, which is send as agent in HTTP request */
#define AGENTSTRING "NTRIP LefebureNTRIPClient"
#define REVISIONSTRING "20131124"
#define MAXDATASIZESEND 1000 /* max number of bytes we can get at once */
#define MAXDATASIZERCV 2024 /* max number of bytes we can get at once */

const char encodingTable [64] = {
'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};


    class NtripCredentials 
    {
        public:
            std::string host;
            int port;
            std::string user;
            std::string password;
            std::string mountpoint;
        
            NtripCredentials(std::string host, int port, std::string user, std::string password, std::string mountpoint);
            NtripCredentials(NtripCredentials& credentials);
    };

    /**
     * @brief NTRIP client that connects to the NTRIP server and receives the RTK corrections.
     * 
     */
    class Ntrip : public Utils::Peripheral::Socket
    {
        private:
            NtripCredentials* credentials;
            Utils::Peripheral::Serial* serial;

            /** @brief Delay between sending GGA messages to the NTRIP server. */
            int delay;
            /** @brief Time when the NTRIP client has previously sent a GGA message. */
            std::chrono::system_clock::time_point ggaSentTime;

            /** @brief Encode the credentials and add them to the buffer */
            int encodeCredentials(char *buf, int size, const char *user, const char *pwd);
            /** @brief Empty the buffer */
            void emptybuf(char* buf, int size);
        public:
            Ntrip(NtripCredentials* credentials, Utils::Peripheral::Serial* serial, int delay);
            ~Ntrip() = default;

            void init() override;
            void run() override;

            /**
             * @brief Sends a GGA message to the NTRIP server. The NTRIP server needs this to optimize the RTK corrections.
             * @details The GGA message is sent repeatedly to the NTRIP server.
             * @param firstTime Parameter to force the GGA message to be sent immediately.
             * @return true 
             * @return false 
             */
            bool sendGga(bool firstTime=false);
    };

} // Core
} // Ilvo