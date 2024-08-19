cmake_minimum_required(VERSION 3.0)
project(unit-tests)

##########################
## Build                ##
##########################
add_executable(test-shape "ShapeTest.cpp")
target_link_libraries(test-shape ilvo-settings-utils)

add_executable(test-polygon "PolygonTest.cpp")
target_link_libraries(test-polygon ilvo-settings-utils)

add_executable(test-platform-settings "PlatformSettingsTest.cpp")
target_link_libraries(test-platform-settings ilvo-settings-utils ilvo-logging-utils)

add_executable(test-transform "TransformTest.cpp")
target_link_libraries(test-transform ilvo-settings-utils)

add_executable(test-transform-order "TransformTestOrder.cpp")
target_link_libraries(test-transform-order ilvo-settings-utils)

add_executable(test-logging "LoggerTest.cpp")
target_link_libraries(test-logging ilvo-logging-utils)

add_executable(test-gps "GpsTest.cpp")
target_link_libraries(test-gps ilvo-gps-utils ilvo-settings-utils)

add_executable(test-arc "ArcTest.cpp")
target_link_libraries(test-arc ilvo-settings-utils)

add_executable(test-line "LineTest.cpp")
target_link_libraries(test-line ilvo-settings-utils)

add_executable(test-traject "TrajectTest.cpp")
target_link_libraries(test-traject ilvo-settings-utils ilvo-redis-utils)