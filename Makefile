NAME = cpu-blue
NAME_TEST = $(NAME)_test
EXEC_LIB  = $(PATH_BIN)/$(NAME)
EXEC_TEST = $(PATH_BIN)/$(NAME_TEST)

CC            = gcc
FLAGS         = -std=c11 -Wall -Wpedantic -Wextra -fsanitize=address # TODO: Remove until i found out how to use it
FLAGS_DEBUG   = $(FLAGS) -ggdb -g3 -O0
FLAGS_RELEASE = $(FLAGS) -O2 -g2

PATH_PRJ  = $(CURDIR)
PATH_SRC  = $(PATH_PRJ)/src
PATH_INCL = $(PATH_PRJ)/include
PATH_BLD  = $(PATH_PRJ)/bld
PATH_TEST = $(PATH_PRJ)/tests
PATH_BIN  = $(PATH_PRJ)/bin

$(PATH_BLD)/%.o: $(PATH_SRC)/%.c
	$(CC) $< -c $(FLAGS_ALL) -o $@

$(EXEC_TEST): $(OBJ_ALL)
	$(CC) $? -o $(PATH_BIN)/$(NAME_TEST)

$(EXEC_LIB): $(OBJ_CARGP)
	ar r $(PATH_BIN)/libcargp.a cargp.o
	ranlib libcargp.a
	$(CC) $^ -o $(PATH_BIN)/$(NAME)

.PHONY: test all clean

# TODO: Add valgrind

test: $(EXEC_TEST)
	cppcheck --enable=all $(PATH_SRC)/*.c

# Creates static library
library: $(EXEC_LIB)

# TODO: Create shared library rule

clean:
	rm -f $(PATH_BLD)/*
	rm -f $(PATH_BIN)/*
