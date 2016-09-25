#!/bin/sh

emcc -std=c++11 -O3 api.cpp -o api.bc
emcc -std=c++11 -O3 html.cpp -o html.bc
emcc -std=c++11 -O3 main.cpp -o main.bc
emcc -std=c++11 -O3 api.bc html.bc main.bc -o main.js
