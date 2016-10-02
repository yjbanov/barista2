cmake_minimum_required(VERSION 3.6)
project(barista2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# EMSCRIPTEN_ROOT is an environment variable pointing at your local installation
# directory of Emscripten.
include_directories("$ENV{EMSCRIPTEN_ROOT}/emscripten/1.35.0/system/include/emscripten/")

add_library(barista2 api.h api.cpp html.h html.cpp)

add_executable(main main.cpp)
target_link_libraries(main barista2)

add_executable(unittests test.cpp)
target_link_libraries(unittests barista2)
