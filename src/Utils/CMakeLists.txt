cmake_minimum_required(VERSION 3.0)
project(utils)


# find dependencies
find_package( Boost COMPONENTS filesystem REQUIRED )


#######################
## Build base        ##
#######################
## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

file(GLOB loggingUtilsFiles 
  "Logging/*.cpp" 
)
add_library(ilvo-logging-utils STATIC ${loggingUtilsFiles})
target_link_libraries(ilvo-logging-utils 
  ${ADDITIONAL_LINK_LIBRARIES}
)

file(GLOB pidUtilsFiles 
  "Pid/*.cpp" 
)
add_library(ilvo-pid-utils STATIC ${pidUtilsFiles})
target_link_libraries(ilvo-pid-utils 
  ${ADDITIONAL_LINK_LIBRARIES}
)

file(GLOB gpsUtilsFiles
  "Nmea/*.cpp" 
  "Peripheral/*.cpp" 
)
add_library(ilvo-gps-utils STATIC ${gpsUtilsFiles})
target_link_libraries(ilvo-gps-utils PUBLIC
  ${ADDITIONAL_LINK_LIBRARIES}
  ilvo-logging-utils
)

file(GLOB dockerUtilsFiles 
  "Docker/*.cpp" 
)
add_library(ilvo-docker-utils STATIC ${dockerUtilsFiles})
target_link_libraries(ilvo-docker-utils 
  CURL::libcurl 
  base64-thirdparty
  ilvo-logging-utils
)

file(GLOB settingUtilsFiles 
  "Settings/*.cpp" 
  "Geometry/*.cpp" 
  "File/*.cpp" 
  "String/*.cpp" 
)
add_library(ilvo-settings-utils STATIC ${settingUtilsFiles})
target_link_libraries(ilvo-settings-utils PUBLIC
  ${ADDITIONAL_LINK_LIBRARIES}
  ${CMAKE_SOURCE_DIR}/dependencies/${CMAKE_SYSTEM_PROCESSOR}/static/libshp.a
  ilvo-logging-utils
)

file(GLOB redisUtilFiles
    "Redis/*.cpp"
    "Timing/*.cpp" 
    "String/*.cpp" 
)

add_library(ilvo-redis-utils
    ${redisUtilFiles}
)
# target_compile_options(redis-utils PUBLIC ${ADDITIONAL_PUBLIC_COMPILE_OPTIONS})
target_link_libraries(ilvo-redis-utils PUBLIC
  ${ADDITIONAL_LINK_LIBRARIES}
  snap7-thirdparty
  bprinter-thirdparty
  ilvo-logging-utils
)