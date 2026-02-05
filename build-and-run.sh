#!/bin/sh
mkdir -p bin
cc -Wall -Wextra -g asteroid.c $(pkg-config --libs --cflags raylib) -o bin/asteroid
./bin/asteroid