#include <Utils/CmdLine/CmdLineParser.h>
#include <iostream>

using namespace std;
using namespace Ilvo::Utils::CmdLine;

CmdLineArg::CmdLineArg(string name, string shortSymbol, string longSymbol, CmdLineArgType type, string defaultValueStr, string usageStr, bool mandatory) : 
    name(name), shortSymbol(shortSymbol), longSymbol(longSymbol), type(type), usageStr(usageStr), mandatory(mandatory)
{
    if (!defaultValueStr.empty()) {
        setValue(defaultValueStr.c_str());
    }
}

void CmdLineArg::setValue(const char* valueStr)
{
    switch (type)
    {
    case CmdLineArgType::STRING:
        value = string(valueStr);
        break;
    case CmdLineArgType::INT:
        value = stoi(string(valueStr));
        break;
    case CmdLineArgType::DOUBLE:
        value = stod(string(valueStr));
        break;
    case CmdLineArgType::BOOL:
        value = 0;
        break;   
    default:
        break;
    }
}

char* CmdLineParser::getCmdOption(char ** begin, char ** end, const string & option)
{
    char ** itr = find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool CmdLineParser::cmdOptionExists(char** begin, char** end, const string& option)
{
    return find(begin, end, option) != end;
}

CmdLineParser::CmdLineParser(string programName, std::vector<std::shared_ptr<CmdLineArg>> args, char ** begin, char ** end, std::string examples) :
    programName(programName), args(args), examples(examples)
{
    if (cmdOptionExists(begin, end, "-h") || cmdOptionExists(begin, end, "--help")) {
        usage();
    }

    for (shared_ptr<CmdLineArg> arg: args)
    {
        if (cmdOptionExists(begin, end, arg->shortSymbol)) {
            if (arg->type == CmdLineArgType::BOOL) {
                arg->setValue("1");
            } else {
                arg->setValue(getCmdOption(begin, end, arg->shortSymbol));
            }
        } else if (cmdOptionExists(begin, end, arg->longSymbol)) {
            if (arg->type == CmdLineArgType::BOOL) {
                arg->setValue("1");
            } else {
                arg->setValue(getCmdOption(begin, end, arg->longSymbol));
            }
        } else if (arg->mandatory) {
            usage();
        }
    }
}

void CmdLineParser::usage()
{
    cout << "Usage: " << programName << " [OPTION] ..." << endl << endl;
    
    for (auto arg: args)
    {
        cout << "\t" << arg->shortSymbol << ", " << arg->longSymbol << "\t" << arg->usageStr << endl;
    }
    cout << endl << "Examples: " << endl;
    cout << "\t" << examples << endl;
    exit(0);
}
