CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

# Give the project a nice name
PROJECT (serialReader)

###### Begin Qt related part ######
set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)
find_package(Qt5SerialPort)
find_package(Qt5Sql)
###### End Qt related part ######

SET(SERIALREADER_SOURCES Interface.cpp serialReader.cpp)

ADD_EXECUTABLE(${PROJECT_NAME} ${SERIALREADER_SOURCES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
    Qt5::Core
    Qt5::SerialPort
    Qt5::Sql
)
