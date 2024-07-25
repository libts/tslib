#!/usr/bin/env bash

mkdir build && cd build
cmake ../
# or adding configuration options: cmake -Denable-input-evdev=ON ../
cmake --build .
