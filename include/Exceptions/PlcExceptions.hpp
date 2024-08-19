#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <Utils/Redis/Variable.h>
#include <ThirdParty/snap7/snap7.h>

namespace Ilvo {
namespace Exception {

   struct PlcSettingsException : public std::exception
   {
      std::string s;
      PlcSettingsException(std::string missingItem) : 
        s("PLC Config file is malconfigured in config.json. Check the " + missingItem + ".") 
      {}
      ~PlcSettingsException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };
    
   struct PlcDataMalformattedException : public std::exception
   {
      std::string s;
      PlcDataMalformattedException(Ilvo::Utils::Redis::VariablePtr var, int byte_cnt, int bit_cnt, int plc_wirte_size)
      {
        std::stringstream ss;
        ss << "Write plc_data failed for " << *var << " for (byte_cnt, bit_cnt, plc_data_length) : (" << byte_cnt << ", " << bit_cnt << ", " << plc_wirte_size << ")" << std::endl;
        s = ss.str();
      }
      ~PlcDataMalformattedException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct PlcNoSuchDataTypeException : public std::exception
   {
      std::string s;
      PlcNoSuchDataTypeException(Ilvo::Utils::Redis::VariablePtr var)
      {
        std::stringstream ss;
        ss << "No datatype conversion possible for type: " << var->getType() << std::endl;
        s = ss.str();
      }
      ~PlcNoSuchDataTypeException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct PlcWriteException : public std::exception
   {
      std::string s;
      PlcWriteException(longword err)
      {
        std::stringstream ss;
        ss << "Write data to plc failed with status code: " << err  << std::endl;
        s = ss.str();
      }
      ~PlcWriteException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct PlcReadException : public std::exception
   {
      std::string s;
      PlcReadException(longword err)
      {
        std::stringstream ss;
        ss << "Read data from plc failed with status code: " << err  << std::endl;
        s = ss.str();
      }
      ~PlcReadException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct PlcNotFound : public std::exception
   {
      std::string s;
      PlcNotFound(std::string ip) : 
         s("PLC on ip " + ip + " is not found on the current network or the connection failed.")
      {}
      ~PlcNotFound() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

} // Exception
} // Ilvo

