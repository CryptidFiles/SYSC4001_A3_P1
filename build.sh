#!/usr/bin/env bash

# Create bin directory if it doesn't exist, otherwise clear it
if [ ! -d "bin" ]; then
    mkdir bin
else
    rm -f bin/*
fi

# Compile the three schedulers using your actual filenames
g++ -std=c++17 -g -O0 -I . -o bin/interrupts_EP \
    interrupts_101299776_101287534_EP.cpp

g++ -std=c++17 -g -O0 -I . -o bin/interrupts_RR \
    interrupts_101299776_101287534_RR.cpp

g++ -std=c++17 -g -O0 -I . -o bin/interrupts_EP_RR \
    interrupts_101299776_101287534_EP_RR.cpp
