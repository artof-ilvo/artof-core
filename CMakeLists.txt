cmake_minimum_required(VERSION 3.5)
project(artof-ilvo)

# Set C++17 as minimum compiling language (necessary for e.g. redis library)
MESSAGE("System processor ${CMAKE_SYSTEM_PROCESSOR}")
IF ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    set(CMAKE_CXX_STANDARD 23)
ELSE ()
    set(CMAKE_CXX_STANDARD 17)
ENDIF ()
MESSAGE("Compiling for CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD}")

IF (NOT DEFINED CMAKE_INSTALL_LIBDIR)
    SET(CMAKE_INSTALL_LIBDIR lib)
ENDIF ()

SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
        CACHE PATH
        ""
    )

SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin
        CACHE PATH
        ""
    )

SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
        CACHE PATH
        ""
    )

## Compile with ASAN
SET(NO_ASAN 0)

# Enable the address sanitizer on Mac and Linux.
# Turn this off by setting the `NO_ASAN` environment variable to `1`
# Cache the environment variable
if (NOT "$ENV{NO_ASAN}" STREQUAL "")
    if ("$ENV{NO_ASAN}" STREQUAL "1")
        set(NO_ASAN "$ENV{NO_ASAN}" CACHE INTERNAL "Copied from environment variable")
    elseif ("$ENV{NO_ASAN}" STREQUAL "0")
        unset (NO_ASAN CACHE)
    endif()
endif()
if(NOT WIN32 AND NOT NO_ASAN)
    if(EXISTS "/etc/arch-release")
        message(STATUS "Note: Compiling with dynamic asan address sanitization and leak checking (\"Arch, as a rule, doesn't include static libs\")")
        set(CMAKE_C_FLAGS      "${CMAKE_C_FLAGS}      -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS}    -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    else()
        message(STATUS "Note: Compiling with asan address sanitization and leak checking")
        set(CMAKE_C_FLAGS      "${CMAKE_C_FLAGS}      -static-libasan -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS}    -fsanitize=address -static-libasan -fno-omit-frame-pointer")
        set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address -static-libasan -fno-omit-frame-pointer")
    endif()
else()
    message("ASAN disabled: To enable, run: NO_ASAN=0 cmake .")
endif()

## Ignore BOOST deprecated headers
add_definitions("-DBOOST_ALLOW_DEPRECATED_HEADERS")
add_definitions("-DBOOST_BIND_GLOBAL_PLACEHOLDERS")

# Find Package Dependencies
find_package( Boost REQUIRED filesystem system thread REQUIRED)
find_package( CURL REQUIRED)

set(ADDITIONAL_LINK_LIBRARIES ${ADDITIONAL_LINK_LIBRARIES} 
    ${Boost_LIBRARIES}
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# message("ADDITIONAL_PUBLIC_COMPILE_OPTIONS: ${ADDITIONAL_PUBLIC_COMPILE_OPTIONS}")
# message("ADDITIONAL_LINK_LIBRARIES: ${ADDITIONAL_LINK_LIBRARIES}")

##########################
## Include directories  ##
##########################

# Libraries
add_subdirectory(src/ThirdParty)
add_subdirectory(src/Utils)

# Executables
# Core functionality
add_subdirectory(src/Gps)
add_subdirectory(src/Navigation)
add_subdirectory(src/Operation)
add_subdirectory(src/Simulation)
add_subdirectory(src/RobotPlc)
add_subdirectory(src/System)
if(NOT UTEST)
    message("-- Testing disabled")
else()
    message("-- Generating test files ...")
    add_subdirectory(src/UnitTest)
endif()