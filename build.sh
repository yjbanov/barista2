#!/bin/sh

# Compiler command
CC="emcc -std=c++11 -O1"

# Compile libs
$CC api.cpp -o api.bc
$CC html.cpp -o html.bc

# Compile sample app
$CC main.cpp -o main.bc
$CC api.bc html.bc main.bc -o main.js \
  -s EXPORTED_FUNCTIONS="['_RenderApp']"

# Compile tests
$CC test.cpp -o test.bc
$CC api.bc html.bc test.bc -o test.js
