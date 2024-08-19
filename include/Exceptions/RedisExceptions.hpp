#pragma once

#include <string>
#include <vector>
#include <sstream>


namespace Ilvo {
namespace Exception {
   
   struct RedisSetVariableException : public std::exception
   {
      std::string s;
      RedisSetVariableException(std::string variable, std::string value) {
        std::stringstream ss;
        ss << "Setting (key, value): (" << variable << ", " << value << ") FAILED";
        s = ss.str();
      }
      ~RedisSetVariableException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

    struct RedisGetVariableEmpty : public std::exception
   {
      std::string s;
      RedisGetVariableEmpty(std::string variable) {
        std::stringstream ss;
        ss << "Get (nil) value for variable: " << variable;
        s = ss.str();
      }
      ~RedisGetVariableEmpty() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

    struct RedisVariableBadCastExceptions : public std::exception
    {
        std::string s;
        RedisVariableBadCastExceptions(std::string variable, std::string type) {
            std::stringstream ss;
            ss << "Variable " << variable << " cannot be casted to type \'" << type << "\'";
            s = ss.str();
        }
        ~RedisVariableBadCastExceptions() throw () {} // Updated
        const char* what() const throw() { return s.c_str(); }
    };

    struct RedisSetMultipleException : public std::exception
    {
        std::string s;
        RedisSetMultipleException() : s("Lenght of vectors do not match") {}
        ~RedisSetMultipleException() throw () {} // Updated
        const char* what() const throw() { return s.c_str(); }
    };

    struct RedisCommandExectionException : public std::exception
    {
        std::string s;
        RedisCommandExectionException(std::string cmd) : s("Redis command failed: " + cmd) {}
        ~RedisCommandExectionException() throw () {} // Updated
        const char* what() const throw() { return s.c_str(); }
    };

    struct RedisVariableBadDefaultValueCastExceptions : public std::exception
    {
        std::string s;
        RedisVariableBadDefaultValueCastExceptions(std::string variable, std::string type) {
            std::stringstream ss;
            ss << "The default value cannot be casted to " << type << "for variable " << variable;
            s = ss.str();
        }
        ~RedisVariableBadDefaultValueCastExceptions() throw () {} // Updated
        const char* what() const throw() { return s.c_str(); }
    };

    struct RedisTypeNotFoundException : public std::exception
    {
        std::string s;
        RedisTypeNotFoundException(std::string type) : s("Type \'" + type + "\' not found exception") {}
        ~RedisTypeNotFoundException() throw () {} // Updated
        const char* what() const throw() { return s.c_str(); }
    };

    struct RedisNoAccessException : public std::exception
    {
        std::string s;
        RedisNoAccessException(std::string variable, std::string processName) {
            std::stringstream ss;
            ss << "The process \'" + processName + "\' is not the owner of variable:" << variable;
            s = ss.str();
        }
        ~RedisNoAccessException() throw () {} // Updated
        const char* what() const throw() { return s.c_str(); }
    };

    struct RedisNoSuchVariableException : public std::exception
    {
        std::string s;
        RedisNoSuchVariableException(std::string key) {
            std::stringstream ss;
            ss << "There is no redis variable with the name:" << key;
            s = ss.str();
        }
        ~RedisNoSuchVariableException() throw () {} // Updated
        const char* what() const throw() { return s.c_str(); }
    };

    struct RedisMalformedConfigException : public std::exception
    {
        std::string s;
        RedisMalformedConfigException(std::string config_name, const exception& e) {
            std::stringstream ss;
            ss << "The config.json file is plaformed for json attributes \'" + config_name + "\' got error: " << e.what();
            s = ss.str();
        }
        ~RedisMalformedConfigException() throw () {} // Updated
        const char* what() const throw() { return s.c_str(); }
    };
    
} // Exception
} // Ilvo


