cmake_minimum_required(VERSION 3.5)
project(ilvo-system-manager)

##########################
## Build ##
##########################

file(GLOB systemFiles
    "*.cpp"
)

add_executable(${PROJECT_NAME} ${systemFiles})
target_link_libraries(${PROJECT_NAME}
  ${ADDITIONAL_LINK_LIBRARIES}
  ilvo-redis-utils
  ilvo-settings-utils
  ilvo-docker-utils
)


##########################
## Install              ##
##########################

if(DEFINED INSTALL_FOLDER)
  message("-- ${PROJECT_NAME} binary files will be installed in ${INSTALL_FOLDER}")
  install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_FOLDER})
else()
  message("-- ${PROJECT_NAME} binary won't be installed no INSTALL_FOLDER specified")
endif()