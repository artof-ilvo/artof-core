#pragma once

#include <string>
#include <algorithm>
#include <vector>
#include <variant>
#include <memory>

namespace Ilvo {
namespace Utils {
namespace CmdLine {
    enum CmdLineArgType {BOOL, STRING, INT, DOUBLE};

    class CmdLineArg
    {
    public:
        std::string name;
        std::string shortSymbol;
        std::string longSymbol;
        CmdLineArgType type;
        std::string usageStr;
        bool mandatory;
        std::variant<std::string,int,double> value;

        CmdLineArg(std::string name, std::string shortSymbol, std::string longSymbol, CmdLineArgType type, std::string defaultValueStr="", std::string usageStr="", bool mandatory=false);
        ~CmdLineArg() = default;

        void setValue(const char* valueStr);

        template <class T>
        T getValue()
        {
            if constexpr (std::is_same<T, bool>::value) { 
                return get<int>(value) == 1;
            } else if constexpr (std::is_same<T, std::string>::value) {
                return std::get<std::string>(value);
            } else {
                return get<T>(value);
            }
        }

    };

    typedef std::shared_ptr<CmdLineArg> CmdLineArgPtr;

    class CmdLineParser
    {
        private:    
            std::string programName;
            std::string examples;
        public:
            std::vector<std::shared_ptr<CmdLineArg>> args;
            
            CmdLineParser(std::string programName, std::vector<std::shared_ptr<CmdLineArg>>, char ** begin, char ** end, std::string examples="");
            ~CmdLineParser() = default;

            void usage();
            static char* getCmdOption(char ** begin, char ** end, const std::string & option);
            static bool cmdOptionExists(char** begin, char** end, const std::string& option);
    };

} // namespace name
} // namespace name 
} // namespace 


