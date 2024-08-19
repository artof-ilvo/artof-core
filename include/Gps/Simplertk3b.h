/**
 * @file Simplertk3b.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief GPS driver for a Septentrio mosaic-H chip (Serial). E.g. module simpleRTK3B Heading.
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO) 
 * 
 */

#pragma once

#include <Utils/Peripheral/Serial.h>
#include <Gps/Ntrip.h>

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

#include <boost/algorithm/string.hpp>

namespace Ilvo {
namespace Core {

    class Simplertk3b : public Utils::Peripheral::Serial
    {

    private:
        std::unique_ptr<Ntrip> ntrip;
        NtripCredentials credentials;
    public:
        Simplertk3b(std::string serialportname, NtripCredentials& ntripCredentials);
        Simplertk3b(const Simplertk3b&) = delete; //Delete the copy constructor
        Simplertk3b& operator=(const Simplertk3b&) = delete; //Delete the Assignment opeartor
        ~Simplertk3b();

        void init() override;
        void run() override;
    };

} // Ilvo
} // Core


