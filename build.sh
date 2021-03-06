#!/bin/sh

set -e
set -x

# Compiler command
EMCC=`which emcc`
echo "Building using $EMCC"
CC="emcc -std=c++11 -O3 -s ASSERTIONS=1 -s NO_EXIT_RUNTIME=1"

# Compile libs
$CC lib/json/src/json.hpp -o json.bc
$CC sync.cpp -o sync.bc
$CC api.cpp -o api.bc
$CC style.cpp -o style.bc
$CC html.cpp -o html.bc

# Compile sample app
$CC main.cpp -o main.bc
$CC json.bc sync.bc api.bc style.bc html.bc main.bc -o main.js \
  -s EXPORTED_FUNCTIONS="['_RenderFrame', '_DispatchEvent', '_main']"

# Compile tests
$CC test.cpp -o test.bc
$CC test_all.cpp -o test_all.bc
$CC json.bc sync.bc api.bc style.bc html.bc test.bc test_all.bc -o test_all.js
