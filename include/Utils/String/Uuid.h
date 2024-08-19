/**
 * @file Uuid.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Extended uuid functionality
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <random>
#include <sstream>

namespace Ilvo {
namespace Utils {
namespace File {

std::string generateUuidV4();

} // namespace Ilvo
} // namespace Utils
} // namespace File