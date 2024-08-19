#include <Gps/Ntrip.h>
#include <Utils/Logging/LoggerStream.h>

using namespace Ilvo::Core;
using namespace Ilvo::Utils::Peripheral;
using namespace Ilvo::Utils::Logging;

using namespace std;
using namespace chrono_literals;

NtripCredentials::NtripCredentials(string host, int port, string user, string password, string mountpoint) :
    host(host),
    port(port),
    user(user),
    password(password),
    mountpoint(mountpoint)
{}

NtripCredentials::NtripCredentials(NtripCredentials& credentials) :
    host(credentials.host),
    port(credentials.port),
    user(credentials.user),
    password(credentials.password),
    mountpoint(credentials.mountpoint)
{}

Ntrip::Ntrip(NtripCredentials* credentials, Serial* serial, int delay) :
    Socket(credentials->host, credentials->port),
    credentials(credentials),
    serial(serial),
    delay(delay),
    ggaSentTime(chrono::system_clock::now())
{
}

void Ntrip::init() {
    // Initialize mountpoint
    int i;
    char bufsend[MAXDATASIZESEND];
    emptybuf(bufsend, MAXDATASIZESEND);
    int spacedoublenewline = 5;

    if(credentials->mountpoint.empty()) {
        i = snprintf(bufsend, MAXDATASIZESEND,
        "GET / HTTP/1.1\r\n"
        "User-Agent: %s/%s\r\n"
        "Accept: */*\r\n"
        "Connection: close \r\n"
        "\r\n"
        , AGENTSTRING, REVISIONSTRING);
    } else {
        i=snprintf(bufsend, MAXDATASIZESEND-40-spacedoublenewline, /* leave some space for login */
            "GET /%s HTTP/1.1\r\n"
            // "ntrip-Version: ntrip/2.0\r\n"
            "User-Agent: %s/%s\r\n"
            // "Accept: */*\r\n"
            // "Connection: close \r\n"
            "Authorization: Basic "
            , credentials->mountpoint.c_str(), AGENTSTRING, REVISIONSTRING);
        if(i > MAXDATASIZESEND-40-spacedoublenewline && i < 0) /* second check for old glibc */ {
            LoggerStream::getInstance() << INFO << "Requested mountpoint too long";
            return;
        }
        i += encodeCredentials(bufsend+i, MAXDATASIZESEND-i-spacedoublenewline, credentials->user.c_str(), credentials->password.c_str());
        if(i > MAXDATASIZESEND-5) {
            LoggerStream::getInstance() << WARN << "Username and/or password too long";
            return;
        }
        snprintf(bufsend+i, spacedoublenewline, "\r\n\r\n");
        i += spacedoublenewline;
    }
    if(send(fd, bufsend, i, 0) != i) {
        LoggerStream::getInstance() << WARN << "send failed";
        return;
    }
    printf("%s",bufsend);

    // send first gga line
    sendGga(true);

    return;
}

void Ntrip::run() {
    int numbytes;
    char bufrecv[MAXDATASIZERCV];
    emptybuf(bufrecv, MAXDATASIZERCV);
    int readfails = 0;

    if(!credentials->mountpoint.empty()) {
        while(RUNNING.load()) {
            numbytes=recv(fd, bufrecv, MAXDATASIZERCV-1, 0);
            if (numbytes > 0) {
                if (numbytes == 14) {
                    LoggerStream::getInstance() << INFO << "Received NTRIP data: " << string(bufrecv);
                }
                // LoggerStream::getInstance() << INFO << "Received NTRIP data: " << string(bufrecv);

                serial->writeRtcm(bufrecv, numbytes);

                emptybuf(bufrecv, MAXDATASIZERCV);
            } 
            else
            {
                LoggerStream::getInstance() << INFO << "(" << readfails << "/50) NTRIP connection attempts. Trying to restart the NTRIP server";
                this_thread::sleep_for(10s);
                readfails++;
                if (readfails > 50) {
                    LoggerStream::getInstance() << INFO << "More than 100 NTRIP connection failures occured, breaking down this thread";
                    break;
                };
            }
            
        }
    } else { // print SOURCETABLE
        while((numbytes=recv(fd, bufrecv, MAXDATASIZERCV-1, 0)) != -1) {
            fwrite(bufrecv, numbytes, 1, stdout);
            if(!strncmp("ENDSOURCETABLE\r\n", bufrecv+numbytes-16, 16)) { break; }
        }
    }

    stop();
    LoggerStream::getInstance() << INFO << "NTRIP connection closed: No data received anymore!";
}

/* does not buffer overrun, but breaks directly after an error */
/* returns the number of required bytes */
int Ntrip::encodeCredentials(char *buf, int size, const char *user, const char *pwd) {
    unsigned char inbuf[3];
    char *out = buf;
    int i, sep = 0, fill = 0, bytes = 0;
    while(*user || *pwd) {
        i = 0;
        while(i < 3 && *user) inbuf[i++] = *(user++);
        if(i < 3 && !sep) {inbuf[i++] = ':'; ++sep; }
        while(i < 3 && *pwd) inbuf[i++] = *(pwd++);
        while(i < 3) {inbuf[i++] = 0; ++fill; }
        if(out-buf < size-1) *(out++) = encodingTable[(inbuf [0] & 0xFC) >> 2];
        if(out-buf < size-1) *(out++) = encodingTable[((inbuf [0] & 0x03) << 4) | ((inbuf [1] & 0xF0) >> 4)];
        if(out-buf < size-1) {
            if(fill == 2) *(out++) = '=';
            else *(out++) = encodingTable[((inbuf [1] & 0x0F) << 2) | ((inbuf [2] & 0xC0) >> 6)];
        }
        if(out-buf < size-1) {
            if(fill >= 1) *(out++) = '=';
            else *(out++) = encodingTable[inbuf [2] & 0x3F];
        }
        bytes += 4;
    }
    if(out-buf < size) *out = 0;
    // printf("%s\n",buf);
    return bytes;
}

bool Ntrip::sendGga(bool firstTime) {
    auto curtime = chrono::system_clock::now();
    int seconds = chrono::duration_cast<chrono::seconds>(curtime - ggaSentTime).count();
    
    // send gga string every delay time
    if ((seconds > delay) || firstTime) { 
        // retreive ggaline
        string ggaline = serial->getGgaLine();

        // send gga line to NTRIP server
        if (fd == -1) {
            LoggerStream::getInstance() << WARN << "File descriptor for NTRIP connection failure";
            return false;
        } else {
            int size = ggaline.length();
            // TODO sometimes crashes on this, make sure ggaline is not empty
            int numbytes = send(fd, ggaline.c_str(), size, 0);
            if(numbytes != size) {
                LoggerStream::getInstance() << INFO << "Send NTRIP GGA line failed";
            } else {
                LoggerStream::getInstance() << INFO << "Successfully sent GGA line to NTRIP server: " << ggaline;
            }
        }

        // update the ggaSentTime
        this->ggaSentTime = curtime;
    }


    return true;
}

void Ntrip::emptybuf(char* buf, int size) {
    for (int i=0; i < size; i++) {
        buf[i] = '\0';
    }
}
