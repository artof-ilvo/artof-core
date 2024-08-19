/**
 * @file Stonex.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief GPS driver for a Stonex GPS (Ethernet). E.g. module simpleRTK3B Heading.
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <Utils/Peripheral/Socket.h>

namespace Ilvo {
namespace Core {

    class Stonex: public Utils::Peripheral::Socket
    {
    private:
        /* data */
    public:
        Stonex(std::string host, int port);
        Stonex(const Stonex&) = delete; //Delete the copy constructor
        Stonex& operator=(const Stonex&) = delete; //Delete the Assignment opeartor
        ~Stonex();

        void init() override;
        void run() override;
    };

} // Core
} // Ilvo