cmake_minimum_required(VERSION 3.21)
project(SCAPMS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_subdirectory(utils)
add_subdirectory(model)
add_subdirectory(controller)
add_subdirectory(gui)
