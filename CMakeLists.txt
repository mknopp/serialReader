CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Give the project a nice name; optional (used for an IDE)
PROJECT (SerialReader)

###### Begin Qt related part ######
# Include Qt 4
FIND_PACKAGE(Qt4 REQUIRED)
# We don't use widgets, so save space and avoid linking to X-Server-libs
SET(QT_DONT_USE_QTGUI TRUE)
SET(QT_USE_QTSQL TRUE)
# QT_USE_FILE adds support for macros like Q_OBJECT among other useful things
INCLUDE(${QT_USE_FILE})
# without the no_keyword option, Qt's signal macro clashes with Boost.Signals
# One has to use 'Q_SIGNALS' and 'Q_SLOTS' instead of 'signals' and 'slots'
#ADD_DEFINITIONS(-DQT_NO_KEYWORDS)
###### End Qt related part ######

	

#ADD_DEFINITIONS(-D__linux__ -DQESP_NO_UDEV)
SET(SERIALREADER_SOURCES Interface.cpp serialReader.cpp qextserialport/src/qextserialport.cpp qextserialport/src/posix_qextserialport.cpp)
SET(SERIALREADER_MOC_SOURCES Interface.h qextserialport/src/qextserialport.h)
QT_WRAP_CPP(serialReader SERIALREADER_SOURCES ${SERIALREADER_MOC_SOURCES})

ADD_EXECUTABLE(serialReader ${SERIALREADER_SOURCES})
TARGET_LINK_LIBRARIES(serialReader ${QT_LIBRARIES})
