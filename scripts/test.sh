# build script for lulz and lalz.
# nothing fancy but many thing from here will be used in the makefile

flags='-std=c11 -Wall -Wpedantic -Wextra -ggdb -g3 -O0'

[ ! -f "./bin/" ] && mkdir -v ./bin/ 2>/dev/null
[ ! -f "./build/" ] && mkdir -v ./build/ 2>/dev/null

rm -f ./bin/*
rm -f ./build/*

gcc $flags -c ./src/cpu.c -o ./build/cpu.o &&
gcc $flags -c ./tests/tests.c -o ./build/tests.o &&
gcc $flags ./build/cpu.o ./build/tests.o -o ./bin/test &&
./bin/test
