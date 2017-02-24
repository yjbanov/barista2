cmake_minimum_required(VERSION 3.6)
project(barista2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

# EMSCRIPTEN_ROOT is an environment variable pointing at your local installation
# directory of Emscripten.
include_directories("$ENV{EMSCRIPTEN_ROOT}/emscripten/1.35.0/system/include/")

add_library(libbarista2 lib/json/src/json.hpp sync.h sync.cpp api.h api.cpp html.h html.cpp style.h style.cpp common.h)

add_executable(main main.cpp)
target_link_libraries(main libbarista2)

add_library(libtest test.h test.cpp)
target_link_libraries(libtest libbarista2)

add_executable(unittests test_all.cpp)
target_link_libraries(unittests libbarista2 libtest)

# Generated giant app test
add_library(libgiantwidgets giant_widgets.h)
set_target_properties(libgiantwidgets PROPERTIES LINKER_LANGUAGE CXX)
add_executable(test_giant test_giant.cpp)
target_link_libraries(test_giant libtest)
