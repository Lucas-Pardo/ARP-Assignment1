#! /usr/bin/bash

gcc src/inspection_console.c -lncurses -lm -o bin/inspection
gcc src/command_console.c -lncurses -o bin/command
gcc src/master.c -o bin/master
gcc src/motorx.c -o bin/motorx
gcc src/motorz.c -o bin/motorz
gcc src/watchdog.c -o bin/watchdog
gcc src/world.c -o bin/world

echo "Compiled."