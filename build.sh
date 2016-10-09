#!/bin/sh

set -e

# Compiler command
CC="emcc -std=c++11 -O3 -s ASSERTIONS=1 -s NO_EXIT_RUNTIME=1"

# Compile libs
$CC api.cpp -o api.bc
$CC style.cpp -o style.bc
$CC html.cpp -o html.bc

# Compile sample app
$CC main.cpp -o main.bc
$CC api.bc style.bc html.bc main.bc -o main.js \
  -s EXPORTED_FUNCTIONS="['_RenderFrame', '_DispatchEvent', '_main']"

# Compile tests
$CC test.cpp -o test.bc
$CC api.bc html.bc test.bc -o test.js
