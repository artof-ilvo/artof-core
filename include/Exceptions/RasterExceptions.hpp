#pragma once

#include <string>
#include <vector>
#include <sstream>


namespace Ilvo {
namespace Exception {

   struct GeoTiffException : public std::exception
   {
      std::string s;
      GeoTiffException(const std::string& filename) : s("There is no GeoTiff found in file " + filename + ".") {}
      ~GeoTiffException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct GeoTransformException : public std::exception
   {
      std::string s;
      GeoTransformException() : s("There is no geo transformation found in the dataset.") {}
      ~GeoTransformException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct InvertGeoTransformException : public std::exception
   {
      std::string s;
      InvertGeoTransformException() : s("The geo transformation cannot be inverted.") {}
      ~InvertGeoTransformException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

} // Exception
} // Ilvo
