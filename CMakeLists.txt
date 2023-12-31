cmake_minimum_required(VERSION 3.0.0)
project(bluetooth VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

set(CMAKE_AUTOMOC ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Bluetooth Widgets REQUIRED)


file(GLOB bluetooth_SRC
     "*.h"
     "*.cpp"
)

add_executable(bluetooth_socket_server ${bluetooth_SRC} pugixml-1.13/src/pugixml.hpp pugixml-1.13/src/pugiconfig.hpp pugixml-1.13/src/pugixml.cpp)

target_link_libraries(bluetooth_socket_server Qt${QT_VERSION_MAJOR}::Core Qt5::Bluetooth Qt5::Widgets)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack) 