#include <Utils/String/String.h>

using namespace std;

vector<int> Ilvo::Utils::String::getIndexList(string msg, char s) 
{ 
    vector<int> result;

    int cnt = 0;
    for (auto &ch : msg) { 
        if (ch == s) { 
            result.push_back(cnt);
        } 
        cnt++;
    } 

    return result;
} 

string Ilvo::Utils::String::toUpperCase(const string& s) {
    string result = "";
    for(int i = 0; i < s.length(); i++) {
        char letter = s[i];
        if(islower(letter)) {
            letter = isupper(letter);
        }
        result.push_back(letter);
    }

    return result;
}

std::string Ilvo::Utils::String::toLowerCase(const std::string& str) {
    std::string result;
    result.reserve(str.length()); // Reserve space for efficiency

    for (char c : str) {
        result.push_back(std::tolower(c)); // Convert each character to lowercase
    }

    return result;
}

string Ilvo::Utils::String::trim(string& in) {
    in.erase(in.find_last_not_of(" \n\r\t")+1);
    return in;
}

string Ilvo::Utils::String::getHeartbeatVariableName(string ilvoProcessName)
{
    string s(ilvoProcessName);
    s.erase(0, string("ilvo-").length());
    return "pc." + s + ".heartbeat";
}