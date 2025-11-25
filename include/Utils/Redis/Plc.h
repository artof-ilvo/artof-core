/**
 * @file Plc.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Abstraction layer for a Siemens PLC (Snap7)
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <ThirdParty/json.hpp>
#include <ThirdParty/ieee754_types.hpp>
#include <ThirdParty/snap7/snap7.h>
#include <Exceptions/PlcExceptions.hpp>
#include <boost/asio.hpp>

namespace Ilvo {
namespace Utils {
namespace Redis {
    class Plc
    {    
    protected:
        std::string ip;


        template<typename T>
        static T getRequired(const nlohmann::ordered_json& j, const std::string& key) {
            if (!j.contains(key)) {
                throw Ilvo::Exception::PlcSettingsException(key);
            }
            return j[key].get<T>();
        }

    public:
        Plc(std::string ip);
        Plc(nlohmann::ordered_json& j);
        ~Plc() = default;

        virtual void read(int size, unsigned char* data) = 0;
        virtual void write(int size, unsigned char* data) = 0;

        const std::string& getIp();
        virtual bool connected();
    };

    class S7Plc: public Plc
    {
        private:
            TS7Client plcClient;
            int readDb;
            int writeDb;
            uint8_t rack;
            uint8_t slot;
        public:
            S7Plc(std::string ip, int readDb, int writeDb, uint8_t rack = 0, uint8_t slot = 1);
            S7Plc(nlohmann::ordered_json& j);
            ~S7Plc();

            void read(int size, unsigned char* data);
            void write(int size, unsigned char* data);
            bool connected();
    };


    class UdpPlc: public Plc
    {
        private:
            boost::asio::io_context io;
            boost::asio::ip::udp::socket socketRead;
            boost::asio::ip::udp::socket socketWrite;
            int readPort;
            int writePort;
            boost::asio::ip::udp::endpoint readerEndpoint;
            boost::asio::ip::udp::endpoint writerEndpoint;
        public:
            UdpPlc(std::string ip, int readPort, int writePort);
            UdpPlc(nlohmann::ordered_json& j);
            ~UdpPlc();

            void read(int size, unsigned char* data);
            void write(int size, unsigned char* data);
    };

} // Redis
} // Utils
} // Ilvo