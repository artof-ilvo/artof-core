#pragma once

#include <string>
#include <vector>
#include <sstream>


namespace Ilvo {
namespace Exception {

   struct TrajectNotLoadedException : public std::exception
   {
      std::string s;
      TrajectNotLoadedException() : s("The traject is not loaded yet, check if the method \'load\' is called your \'Traject\' instance.") {}
      ~TrajectNotLoadedException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct SectionOutOfIndexException : public std::exception
   {
      std::string s;
      SectionOutOfIndexException(int i, int j) : s("Number: " + std::to_string(i) + " is out of the number of sections: " + std::to_string(j)) {}
      ~SectionOutOfIndexException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct HitchNotFoundException : public std::exception
   {
      std::string s;
      HitchNotFoundException(const std::string& hitch_name) : s("No hitch with name " + hitch_name + " found in the \'settings.json\' file.") {}
      ~HitchNotFoundException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct NoNavigationAlgorithmIdFound : public std::exception
   {
      std::string s;
      NoNavigationAlgorithmIdFound(int nav_algo_id) : s("No navigation algorithm found with id \'" + std::to_string(nav_algo_id) + "\' check if the navigation algorithm id's in \'settings.json\' file correspond with the \'AlgorithmMode\' enumeration in \'controller.h\'.") {}
      ~NoNavigationAlgorithmIdFound() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct WrongRobotOrientation : public std::exception
   {
      std::string s;
      WrongRobotOrientation() : s("The robot is wrong oriented relative to the traject!") {}
      ~WrongRobotOrientation() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };


   struct TrajectLengthIsZero : public std::exception
   {
      std::string s;
      TrajectLengthIsZero() : s("The Traject length is zero!") {}
      ~TrajectLengthIsZero() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct TrajectLost : public std::exception
   {
      std::string s;
      TrajectLost() : s("The traject was lost! Place the robot close to the traject.") {}
      ~TrajectLost() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct TrajectIndexException : public std::exception
   {
      std::string s;
      TrajectIndexException(int index, int size) : s("The traject index " + std::to_string(index) + " exceeds the size of the traject: " + std::to_string(size) + "") {}
      ~TrajectIndexException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct RobotOutsideGeofence : public std::exception
   {
      std::string s;
      RobotOutsideGeofence() : s("The Robot stopped with auto mode because it is outside the geofence.") {}
      ~RobotOutsideGeofence() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct EndOfTrajectIsReached : public std::exception
   {
      std::string s;
      EndOfTrajectIsReached() : s("The Robot stopped with auto mode because it the end of the traject is reached.") {}
      ~EndOfTrajectIsReached() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct NoRtkFix : public std::exception
   {
      std::string s;
      NoRtkFix() : s("The Robot stopped with auto mode because there is no RTK fix.") {}
      ~NoRtkFix() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

   struct SettingsParamNotFoundException : public std::exception
   {
      std::string s;
      SettingsParamNotFoundException(std::string parent, std::string param) : s("Unable to find the parameter \"" + param + "\" in the \"" + parent + "\" object of settings.json file") {}
      ~SettingsParamNotFoundException() throw () {} // Updated
      const char* what() const throw() { return s.c_str(); }
   };

} // Exception
} // Ilvo
