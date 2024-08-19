#pragma once

#include <string>
#include <vector>
#include <sstream>


namespace Ilvo {
namespace Exception {
   
   struct NmeaException : public std::exception
   {
      std::string s;
      NmeaException(std::string& name) : s("No attribute " + name + ". Check nmea_message.h.") {}
      ~NmeaException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

} // Exception
} // Ilvo
