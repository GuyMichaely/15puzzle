all: npuzzle test

npuzzle: main.c inversions.h
	gcc main.c -o main -lncurses -Dconst=

test: test.c inversions.h
	gcc test.c -o test -lncurses 
