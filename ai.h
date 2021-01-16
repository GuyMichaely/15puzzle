#include <ncurses.h>
#include <unistd.h>
#include <setjmp.h>

#include "drawing.h"
#include "undo.h"
#include "swap_ints.h"
#include "game_vars.h"
#include "test.h"

#define MOVE_DELAY_MS -1

typedef struct Coordinate{
	int y;
	int x;
} Coordinate;

jmp_buf exitAi; // buffer used for exiting ai when user hits 'c'

static inline void clearMsg(const int y, char *msg) {
	timeout(MOVE_DELAY_MS);
	mvhline(y, (COLS - strlen(msg)) / 2, ' ', strlen(msg));
}

static inline void clearMsgs() {
	nodelay(stdscr, true); // causes getch() to be non blocking
	clearMsg(0, "What algorithm should be used to solve?");
	clearMsg(1, "0: Just for fun inefficient algorithm");
	clearMsg(2, "1: A* with linear conflict + manhattan distance as heuristic");
	clearMsg(3, "2: https://www.aaai.org/Papers/JAIR/Vol22/JAIR-2209.pdf");

}

// sleeps and returns getch() == 'q' or 'Q'
char statusGetch() {
	const int c = getch();
	switch (c) {
		case 'q':
		case 'Q':
			return 'q';
		case 'c':
		case 'C':
			nodelay(stdscr, false);
			return 'c';
	}
	return false;
}

// the algorithm to solve a row vs a column are the same, just transposed
// hence, I will implement a function capable of solving a column
// with the aid of seperate functions that get and set cells
// then I will create 2 sets of functions for working with cells:
// one for normal coordinates and the other for transposed

// stores coordinate of v in *coord
void getRealCoord(int v, GameVars *game, int coordinates[], Coordinate *coord) {
	const int coord0 = game->cols * game->y + game->x;
	if (v > coord0) {
		v--;
	}
	coord->y = coordinates[v] / game->cols;
	coord->x = coordinates[v] % game->cols;
}

// stores coordinate of v in *coord
void getTransposedCoord(int v, GameVars *game, int coordinates[], Coordinate *coord) {
	getRealCoord(v, game, coordinates, coord);
	swapInts(&(coord->y), &(coord->x));
}

int *returnFirst(int *a, int *b){ return a; } // pass in to funAiEngine's transformCoord when solving with real coords
// static inline void decrementFirst (int *f, int *s) { (*f)--; }
// static inline void decrementSecond(int *f, int *s) { (*s)--; }

void realSwap(GameVars *game, int swapy, int swapx, Move** undo, char direction) {
	swap0(game, swapy, swapx, undo, direction);
	switch (statusGetch()) { 
		case 'c':
			longjmp(exitAi, 1);
		case 'q':
			endwin();
			exit(0);
	}
}

void transposeSwap(GameVars *game, int swapy, int swapx, Move** undo, char direction) {
	realSwap(game, swapx, swapy, undo, direction);
}

// assumes starting to the right of the cell
void upRight(GameVars *game, Move** undo, void (*swapf)(GameVars*, int, int, Move**, char), int n) {
	while (n--) {
		swapf(game, 0, -1, undo, 'r');
		swapf(game, 1, 0, undo, 'd');
		swapf(game, 0, 1, undo, 'l');
		if (n--) {
			swapf(game, -1, 0, undo, 'u');
			swapf(game, 0, 1, undo, 'l');
			swapf(game, 0, 1, undo, 'd');
		}
	}
}

// happens in 2 phases:
// 	1. move 0 to the side of A it needs to be on
// 		* if A is in a 45 deg diagonal to B, move 0 to whichever prouctive side
// 		  of A is closest
// 		* otherwise move it so the number of moves where A is moved 
// 		  horizontally/vertically is minimised
// 	2. move A diagonally until it's either in the same row or column as B
// 	3. Move A in a horizontal/vertical line to B if neccesary
void moveAToB(GameVars *game, Move** undo, Coordinate *a, Coordinate *b, int* (transformInts)(int*, int*), void (*swapf)(GameVars*, int, int, Move**, char)) {
	transformInts(&(game->x), &(game->y)); // must undo at end of function lest the real coordinates will be transposed
	int dy, dx;
	const int vertDist = a->y - b->y;
	const int horDist = a->x - b->x;
	if (horDist < vertDist * (vertDist < 0 ? -1 : 1)) {
		// horizontal distance < vertical distance
		
		// ensure 0 is not in same row as A before we move right
		//
		if (game->y == a->y) { // 0 in the same row as A
			if (game->x < a->x) { // 0 to the left of A
				if (game->y) { // there is a row above; move up to go around 
					swapf(game, -1, 0, undo, 'd');
				}
				else { // we are at the top row; move down to go around
					swapf(game, 1, 0, undo, 'u');
				}
			}
		}
		// 
		// 0 is now not in the same row and to the left of A
		
		// ensure 0 is to the right of A
		while (game->x <= a->x) {
			swapf(game, 0, 1, undo, 'l');
		}

		// get 0 to the same row as A
		if (game->y != a->y) {
			if (a->y > game->y) {
				do {
					swapf(game, 1, 0, undo, 'u');
				} while (a->y != game->y);
			}
			else {
				swapf(game, -1, 0, undo, 'd');
			}
		}

		// get 0 directly to the right of A
		while (game->x > a->x) {
			swapf(game, 0, -1, undo, 'r');
		}

	}
	else if (horDist > vertDist * (vertDist < 0 ? -1 : 1)) {
		// horizontal distance < magintude of vert distance
		if (vertDist < 0) {
			// a is below b
			dy = 1;
		}
		else {
			dy = -1;
		}
	}
	else {
		// vertical and horizontal distances are equal
		// where you must go is determined by which side is closer
		//
		// given equal horizontal and vertical distances:
		/*	  |XXXXXXXXXX
		 *  	  |XXXXXXXXXX
		 *   	  |XXXXXXXXXX
		 *        |XXXXXXXXXX
		 *--------+-+XXXXXXXX
		 *XXXXXXXX|A!XXXXXXXX
		 *XXXXXXXX+!+--------
		 *XXXXXXXXXX|
		 *XXXXXXXXXX|
		 *XXXXXXXXXX|	    B
		 *
		 * please excuse the awful illustration, assume the box with corners A and B is a square
		 * A is the src cell and B the goal
		 * if the 0 is in either of the areas marked X there is an optimal side to choose
		 * that side is marked by the corresponding '!'
		 * if A is below B, flip the diagram upside down
		*/
		if (a->y > b->y) {

		}
		else {

		}
	}
	transformInts(&game->x, &game->y);
}

// solve a column/tranposed column of the grid
// solve up to and including (col, row)
// the solve direction is determined by corresponding functions passed in
void funAiEngine(
	GameVars *game, Move **undo, int coordinates[], int row, int col,
	int* (*transformInts)(int*, int*),
	void (*getCoord)(
		int,
		GameVars*,
		int[],
		Coordinate*
	),
	void (*swapf)(
		GameVars*,
		int,
		int,
		Move**,
		char
	)
) {
	for (int *i = transformInts(&row, &col); *i > 1; (*i)--) {
		Coordinate a;
		getCoord(row * game->cols + col, game, coordinates, &a);
		Coordinate b = {row, col};
		transformInts(&(b.y), &(b.x));
		moveAToB(game, undo, &a, &b, transformInts, swapf);
	} 
}

void funAi(GameVars *game, Move **undo) {
	// make array of cell coordinates
	const int length = game->rows * game->cols - 1;
	int coordinates[length];
	int i = 0;
	for (const int zeroCoord = game->y * game->cols + game->x; i < zeroCoord; i++) {
		const int cell = game->cells[i / game->cols][i % game->cols];
		coordinates[cell - 1] = i;
	}

	i++; // skip over 0
	for (; i < length; i++) {
		const int cell = game->cells[i / game->cols][i % game->cols];
		coordinates[cell - 1] = i;
	}

	// make it so there are as many unsolved rows as unsolved columns
	if (game->cols > game->rows) { // if more columns than rows
		int i = game->cols - game->rows;
		do {
			funAiEngine(game, undo, coordinates, game->rows - 1, game->rows - 1 + i, returnFirst, getRealCoord, realSwap);
		} while (--i);
	}
	else if (game->rows < game->cols) {

	}

	// the unsolved area is now a square
	// solve rows and columns one after another until we get to a 2x2
	// for (int i = 0; i < game->rows - 2; i++) {
	//         const int cornerIndex = game->rows - 1 - i;
	//         funAiEngine(game, undo, cornerIndex, cornerIndex);
	//         funAiEngine(game, undo, cornerIndex, cornerIndex - 1);
	// }

	// now solve the 2x2
}

void ai(GameVars *game, Move **undo) {
	// jumped to by transposeSwap and realSwap when user hits 'c'
	if (setjmp(exitAi)) {
		timeout(0);
		return;
	}

	midPrint(0, "What algorithm should be used to solve?");
	midPrint(1, "0: Just for fun greedy algorithm");
	midPrint(2, "1: A* with linear conflict + manhattan distance as heuristic");
	midPrint(3, "2: https://www.aaai.org/Papers/JAIR/Vol22/JAIR-2209.pdf");

	int c;
	while ((c = getch()) != 'c' && c != 'C') {
		switch (c) {
			case 'q':
			case 'Q':
				endwin();
				exit(0);
			case '0':
				clearMsgs();
				funAi(game, undo);
				return;
		}
	}
	clearMsgs();
	return;
}
