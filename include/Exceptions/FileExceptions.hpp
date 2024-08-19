#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace Ilvo {
namespace Exception {
   
   struct PathNotFoundException : public std::exception
   {
      std::string s;
      PathNotFoundException(std::string ss) : s("The file containing the robot path: " + ss + " does not exist") {}
      ~PathNotFoundException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct NoTasksWithTypeFound : public std::exception
   {
      std::string s;
      NoTasksWithTypeFound(std::string type) : s("There are no tasks of type: " + type + ".") {}
      ~NoTasksWithTypeFound() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct EnvVariableNotFoundException : public std::exception
   {
      std::string s;
      EnvVariableNotFoundException(std::string ss) : s("The environment variable: " + ss + " does not exist") {}
      ~EnvVariableNotFoundException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct WrongFileExtensionException : public std::exception
   {
      std::string s;
      WrongFileExtensionException(std::string ext1, std::string ext2) : s("The filename has the wrong extension: " + ext1 + " must be " + ext2) {}
      ~WrongFileExtensionException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct NoShpOrCsvFileException : public std::exception
   {
      std::string s;
      NoShpOrCsvFileException(std::string directory_path) : s("There is no .shp or .csv detected in " + directory_path) {}
      ~NoShpOrCsvFileException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct PolygonWithHoleException : public std::exception
   {
      std::string s;
      PolygonWithHoleException() : s("A polygon with zero or more then one parts detected! Polygons with holes are not permitted!") {}
      ~PolygonWithHoleException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct CsvColumnsNotFoundException : public std::exception
   {
      std::string s;
      CsvColumnsNotFoundException(std::vector<std::string>& xFields, std::vector<std::string>& yFields) 
      {
         std::stringstream ss_xFields;
         ss_xFields << "[";
         for (auto it = xFields.begin(); it != xFields.end(); it++) {
            if (it != xFields.end()-1) {
               ss_xFields << *it << ", ";
            } else {
               ss_xFields << *it;
            }
         }
         ss_xFields << "]";
         std::stringstream ss_yFields;
         ss_yFields << "[";
         for (auto it = yFields.begin(); it != yFields.end(); it++) {
            if (it != yFields.end()-1) {
               ss_yFields << *it << ", ";
            } else {
               ss_yFields << *it;
            }
         }
         ss_yFields << "]";
         s = "The csvfile has no columns: " + ss_xFields.str() + " or " + ss_yFields.str();
      }
      ~CsvColumnsNotFoundException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

} // Exception
} // Ilvo
