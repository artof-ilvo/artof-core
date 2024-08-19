/**
 * @file LoggerStream.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Logging stream
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <map>
#include <iostream>
#include <memory>

#include <boost/filesystem.hpp>


namespace Ilvo {
namespace Utils {
namespace Logging {

    const int LOG_FILE_MAX_SIZE = 10 * 1024 * 1024;  // 10 MB

    enum LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    /**
     * @brief Logging stream
     * 
     * @details This singleton class is used to log messages to a file.
     */
    class LoggerStream 
    {
    private:
        std::string name;
        std::string fName;
        boost::filesystem::path logDir;
        std::ofstream fstream;

        bool terminalOutput;

        std::map<LogLevel, std::string> levelString = {
            {DEBUG, "DEBUG"},
            {INFO, "INFO"},
            {WARN, "WARN"},
            {ERROR, "ERROR"}
        };

        // static instance wich will point to the instance of this class
        static std::shared_ptr<LoggerStream> instancePtr;

        void setFileName();
    public:
        LoggerStream(std::string name, bool terminalOutput = false);
        LoggerStream(const LoggerStream& other) = delete;  // delete copy constructor
        ~LoggerStream();

        static void createInstance(std::string name, bool terminalOutput = false);
        /**
         * @brief Get the Instance object
         * 
         * @return LoggerStream& 
         */
        static LoggerStream& getInstance();

        LoggerStream& operator<< (LogLevel level);
        
        template<typename T>
        LoggerStream& operator<< (const T& s) {
            fstream << s;
            fstream.flush();
            if (terminalOutput) std::cout << s;

            return *this;
        }
    };

}
}
}