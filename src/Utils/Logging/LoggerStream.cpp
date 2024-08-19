#include <Utils/Logging/LoggerStream.h>

using namespace Ilvo::Utils::Logging;

using namespace std;

namespace fs = boost::filesystem;

// initializing instancePtr with NULL
std::shared_ptr<LoggerStream> LoggerStream::instancePtr = NULL; 

LoggerStream::LoggerStream(string name, bool terminalOutput) :
    name(name),
    fName(name + ".log"),
    logDir(fs::path(getenv("ILVO_PATH")) / "logs"),
    terminalOutput(terminalOutput)
{
    fs::create_directories(logDir);
    setFileName();
    fstream.open(logDir / fName, ofstream::out | ofstream::app);
}

LoggerStream::~LoggerStream()
{
    if (fstream.is_open()) {
        fstream.close();
    }
}

void LoggerStream::setFileName() {
    if (fs::exists(logDir / fName)) {
        // When the log file is found, check if it is greater than 10 MB
        if (fs::file_size(logDir / fName) > LOG_FILE_MAX_SIZE) {
            string backupName = fName + "1";
            // remove if suffix file exists
            if (fs::exists(logDir / backupName)) fs::remove(logDir / backupName); // remove suffix file if exists
            fs::copy_file(logDir / fName, logDir / backupName);  // backup file
            fs::remove(logDir / fName);  // remove file
        } 
    }
}

void LoggerStream::createInstance(string name, bool terminalOutput) {
    instancePtr = std::make_shared<LoggerStream>(name, terminalOutput);
}

LoggerStream& LoggerStream::getInstance() {
    if (instancePtr == NULL) {  
        throw runtime_error("Logger not initialized");
        // returning the instance pointer
        return *instancePtr; 
    } else {
        return *instancePtr;
    }
}

LoggerStream& LoggerStream::operator<< (LogLevel level) {
    stringstream logHeader;

    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto ms_part = now_ms.time_since_epoch().count() % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    logHeader << endl << "[" << std::put_time(std::localtime(&t), "%a %Y %b %d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms_part << "][" << levelString[level] << "] ";

    fstream << logHeader.str();
    if (terminalOutput) cout << logHeader.str();

    return *this;
}
