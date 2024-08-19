cmake_minimum_required(VERSION 3.5)
project(ilvo-gps)

#######################
## Build gps      ##
#######################
file(GLOB gpsFiles
    "*.cpp"
)

add_executable(${PROJECT_NAME}
    ${gpsFiles}
)
target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME}
  ${ADDITIONAL_LINK_LIBRARIES}
  ilvo-gps-utils
  ilvo-redis-utils
  ilvo-settings-utils
)


##########################
## Install              ##
##########################

if(DEFINED INSTALL_FOLDER)
  message("-- ${PROJECT_NAME} binary files will be installed in ${INSTALL_FOLDER}")
  install(TARGETS ilvo-gps DESTINATION ${INSTALL_FOLDER})
else()
  message("-- ${PROJECT_NAME} binary won't be installed no INSTALL_FOLDER specified")
endif()

