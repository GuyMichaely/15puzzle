npuzzle: main.c randomization.h
	gcc main.c -o main -lncurses -Dconst=

ntest: main.c randomization.h
	gcc main.c -o main -g -lncurses -Dconst=

test: test.c randomization.h
	gcc test.c -o test -lncurses 

all: npuzzle test
