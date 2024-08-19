/**
 * @file RedisStream.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief Redis client
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */
#pragma once

#include <string>
#include <iostream>
#include <memory>
#include <iomanip>
#include <Utils/String/String.h>
#include <ThirdParty/json.hpp>
#include <ThirdParty/redis-cpp/stream.h>
#include <ThirdParty/redis-cpp/execute.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/thread.hpp>


namespace Ilvo {
namespace Utils {
namespace Redis {

    /**
     * @brief Redis client
     * 
     * @details This class is used to communicate with a Redis server, enabling reading and writing of single variables or json objects.
     */
    class RedisStream
    {
    private:
        /** @brief Redis server ip address */
        std::string ip;
        /** @brief Redis server port */
        int port;
        
        std::shared_ptr<std::iostream> stream;

        std::map<std::string, std::pair<std::shared_ptr<boost::thread>, std::shared_ptr<std::atomic<bool>>>> subscriberThreads;
    public:
        RedisStream() = default;
        RedisStream(nlohmann::json j);
        RedisStream(std::string ip, int port);
        ~RedisStream();

        /** @brief Checks if redis variable exists */
        bool isRedisValueNil(std::string name);

        /** @brief Convert redis command to string */
        template<typename ... Args>
        std::string redisCmdToStr(std::string cmd, Args ... args)
        {
            std::vector<std::string> v;
            v.push_back(cmd);
            (v.push_back(std::string( args )), ...);
            std::string s = boost::algorithm::join(v, " ");
            return s;
        }

        /** @brief Set one or multiple redis variables */
        template<typename ... Args>
        bool setRedisValue(Args... args) 
        {
            if (sizeof ... (args) != 2)  throw Exception::RedisCommandExectionException("Invalid number of arguments for GET");
            std::string cmd = "SET";
            auto response = rediscpp::execute(*(stream), cmd, (std::stringstream() << std::setprecision(10) << args).str() ... );
            // LoggerStream::getInstance() << DEBUG << "Redis: " << redisCmdToStr(cmd, (std::stringstream() << std::setprecision(10) << args).str() ... ) << std::endl;;
            return response.as_string().compare("OK") != 0;
        }

        /** @brief set multiple redis variables based on a vector */
        bool setRedisValues(std::vector<std::string> values);
        rediscpp::deserialization::array::items_type getRedisValues(std::vector<std::string> values);

        /** @brief Get one or multiple redis variables */
        template<typename ... Args>
        std::string getRedisValue(Args... args) { 
            if (sizeof ... (args) != 1) throw Exception::RedisCommandExectionException("Invalid number of arguments for GET");
            std::string cmd = "GET";  
            rediscpp::value response = rediscpp::execute(*(stream), cmd, args ...  );
            // LoggerStream::getInstance() << DEBUG << "Redis: " << redisCmdToStr(cmd, args ... ) << " - ";
            try {
                std::string s = (std::string) response.as_string();
                // LoggerStream::getInstance() << DEBUG << "value: " << s << std::endl;
                return s;
            } catch(std::logic_error&) {
                return "";
            }
        }

        /** @brief Delete one or multiple redis variables */
        template<typename ... Args>
        int delRedisValues(Args&& ... args)
        {
            std::string cmd = "DEL";  
            auto response = rediscpp::execute(*(stream), cmd, args ... );
            return response.as_integer();
        }

        /** @brief Set redis json variable */
        bool setRedisJsonValue(std::string name, const nlohmann::json& value);
        /** @brief Get redis json variable */
        nlohmann::json getRedisJsonValue(std::string name);
        /** @brief Get redis json variable and initialize it if it doesn't exist */
        nlohmann::json getRedisJsonValue(std::string name, nlohmann::json initIfNotExists);

        /** @brief Publish redis variable */
        template <typename T>
        int publishRedisValue(std::string name, T value) {
            std::string valueStr = Utils::String::toRedisString<T>(value);
            auto response = rediscpp::execute(*(stream), "PUBLISH", name, valueStr);
            return response.as<int>();
        }

        /** @brief Subscribe a redis variable */
        void subscribeRedisValue(std::string& name, std::function<void(const std::string_view&)>& callback);
        /** @brief Unsubscribe a redis variable */
        void detachSubscribeRedisValue(std::string& name);
        /** @brief clear subscription record */
        void unsubscribeRedisValue(std::string& name);
    };

}
}
}