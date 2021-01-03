npuzzle: main.c inversions.h
	gcc main.c -o main -lncurses -Dconst=

ntest: main.c inversions.h
	gcc main.c -o main -lncurses -Dconst=

test: test.c inversions.h
	gcc test.c -o test -lncurses 

all: npuzzle test
