cmake_minimum_required(VERSION 3.6)
project(barista2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp api.h api.cpp)
add_executable(barista2 ${SOURCE_FILES})
