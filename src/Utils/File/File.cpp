#include <Utils/File/File.h>
#include <filesystem>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace std::filesystem;
using namespace boost::algorithm;

string Ilvo::Utils::File::searchFileWithExtension(string directory_path, string ext)
{
    for (const auto& entry : recursive_directory_iterator(directory_path))
    {
        if (is_regular_file(entry.status())) 
        {
            string file_ext = entry.path().extension();
            if (file_ext.compare(ext) == 0) {
                stringstream filename_ss, ss;
                filename_ss << entry.path().filename();
                string filename = filename_ss.str();
                // removes brackets if exists
                removeCharacters(filename, {'\"'});
                ss << directory_path << "/" << filename;
                return ss.str();
            }
        }
    }
    return "";
}

void Ilvo::Utils::File::removeCharacters(string& s, const vector<char>& removeChars)
{
    for (char c: removeChars) {
        s.erase(remove(s.begin(), s.end(), c), s.end());
    }
}