cmake_minimum_required(VERSION 3.6)
project(barista2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(barista2 api.h api.cpp)
add_executable(main main.cpp)
target_link_libraries(main barista2)
