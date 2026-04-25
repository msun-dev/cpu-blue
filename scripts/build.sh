# build script for lulz and lalz.
# nothing fancy but many thing from here will be used in the makefile

flags='-std=c11 -Wall -Wpedantic -Wextra -ggdb -g3 -O0' # -fsanitize=address'

rm -f ./bin/*
rm -f ./build/*
gcc $flags -c ./src/cpu.c -o ./build/cpu.o
gcc $flags -c ./tests/tests.c -o ./build/tests.o
gcc $flags ./build/cpu.o ./build/tests.o -o ./bin/test

