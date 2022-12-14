#! /usr/bin/bash

gcc -fdiagnostics-color=always -g src/inspection_console.c -lncurses -lm -o bin/inspection
gcc -fdiagnostics-color=always -g src/command_console.c -lncurses -o bin/command
gcc -fdiagnostics-color=always -g src/master.c -o bin/master
gcc -fdiagnostics-color=always -g src/motorx.c -o bin/motorx
gcc -fdiagnostics-color=always -g src/motorz.c -o bin/motorz
gcc -fdiagnostics-color=always -g src/world.c -o bin/world

echo "Compiled."