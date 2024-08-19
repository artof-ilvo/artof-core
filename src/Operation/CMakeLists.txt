cmake_minimum_required(VERSION 3.5)
project(ilvo-operation)

#######################
## Build base        ##
#######################
file(GLOB operationFiles
  "*.cpp"
)

add_executable(${PROJECT_NAME} ${operationFiles})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ADDITIONAL_LINK_LIBRARIES}
  ilvo-redis-utils
  ilvo-settings-utils
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
