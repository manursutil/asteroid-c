#!/bin/sh
set -eu

mkdir -p bin
cc -Wall -Wextra -g asteroid.c $(pkg-config --libs --cflags raylib) -o bin/asteroid
