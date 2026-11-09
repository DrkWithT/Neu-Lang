#!/bin/bash

option="$1";

if [ "$option" = "help" ]
then
    printf "\x1b[1;33mUSAGE:\x1b[0m\n./project.sh [help | build <preset> <generator>]\n";
    exit 0;
elif [ "$option" = "build" ]
then
    cmake -S . -B build --preset="$2" -G "$3" && cmake --build build && mv ./build/compile_commands.json .;
else
    printf "\x1b[1;33mUSAGE:\x1b[0m\n./project.sh [help | build <preset> <generator>]\n";
    exit 1;
fi
