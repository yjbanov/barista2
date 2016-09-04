#!/bin/sh

emcc -O2 api.h -o api.bc
emcc -O2 main.cpp -o main.bc
emcc -O2 api.bc main.bc -o main.js
