/**
 * @file File.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Extended file functions
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <vector>

namespace Ilvo {
namespace Utils {
namespace File {

    const std::vector<char> CHARS_END_OF_LINE = {'\r', '\n', ' '};
    const std::vector<char> CHARS_BRACKETS = {'\"'};

    /**
     * @brief Finds the first file with the given extension.
     * 
     * @param directory_path: The directory to search in 
     * @param ext: The extension to search for 
     * @return std::string 
     */
    std::string searchFileWithExtension(std::string directory_path, std::string ext);
    
    /**
     * @brief Removes characters from a string
     * 
     * @param s: The string to remove from 
     * @param removeChars: The characters to remove 
     */
    void removeCharacters(std::string& s, const std::vector<char>& removeChars);

} // namespace Ilvo
} // namespace Utils
} // namespace File