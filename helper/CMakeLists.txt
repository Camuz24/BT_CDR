cmake_minimum_required(VERSION 3.0.0)
project(receiver VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

set(CMAKE_AUTOMOC ON)


find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Bluetooth REQUIRED)

add_executable(receiver main.cpp shared_memory.h shared_memory.cpp)
add_executable(writer mainWriter.cpp shared_memory.h shared_memory.cpp)


target_link_libraries(receiver Qt${QT_VERSION_MAJOR}::Core Qt5::Bluetooth)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)