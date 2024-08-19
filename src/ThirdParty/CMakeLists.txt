cmake_minimum_required(VERSION 3.0)
project(thirdparty)

# find dependencies
# find_package(Boost)

##########################
## Build                ##
##########################
include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(bprinter-thirdparty STATIC "bprinter/table_printer.cpp")

add_library(snap7 SHARED IMPORTED)
set_property(TARGET snap7 PROPERTY IMPORTED_LOCATION "/usr/lib/ilvo/libsnap7.so")

add_library(snap7-thirdparty STATIC "snap7/snap7.cpp")
target_link_libraries(snap7-thirdparty snap7)

add_library(base64-thirdparty STATIC "base64/base64.cpp")
