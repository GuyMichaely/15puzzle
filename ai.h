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

typedef void (*CoordGetter)(GameVars*, int, Coordinate*); 
typedef void (*SwapFunction)(GameVars*, int, int, char);

jmp_buf exitAi; // buffer used for exiting ai when user hits 'c'

// swap function used by ai
void swap(GameVars *game, int swapy, int swapx, char undoDirection) {
	// update coordinate of swapped cell
	const int v = game->cells[game->y][game->x];
	int newCoordinate = game->coordinates[v];
	newCoordinate -= game->cols * swapy + swapx;
	game->coordinates[v] = newCoordinate;

	// swap the cells on screen and in game->cells
	swap0(game, swapy, swapx, undoDirection);
}

// clear a splash screen message by index and message content
static inline void clearMsg(const int y, char *msg) {
	timeout(MOVE_DELAY_MS);
	mvhline(y, (COLS - strlen(msg)) / 2, ' ', strlen(msg));
}

// clear the splash screen
static inline void clearMsgs() {
	nodelay(stdscr, true); // causes getch() to be non blocking
	clearMsg(0, "What algorithm should be used to solve?");
	clearMsg(1, "0: Just for fun inefficient algorithm");
	clearMsg(2, "1: A* with linear conflict + manhattan distance as heuristic");
	clearMsg(3, "2: https://www.aaai.org/Papers/JAIR/Vol22/JAIR-2209.pdf");

}

// getch() and return either 'c', 'q', or 0 depending on user input
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
	return 0;
}

// the algorithm to solve a row vs a column are the same, just transposed
// hence, I will implement a function capable of solving a column
// with the aid of seperate functions that get and set cells
// then I will create 2 sets of functions for working with cells:
// one for normal coordinates and the other for transposed

// stores coordinate of v in *coord
void getRealCoord(GameVars *game, int v, Coordinate *coord) {
	coord->y = game->coordinates[v - 1] / game->cols;
	coord->x = game->coordinates[v - 1] % game->cols;
}

// stores coordinate of v in *coord
void getTransposedCoord(int v, GameVars *game, Coordinate *coord) {
	getRealCoord(game, v, coord);
	swapInts(&(coord->y), &(coord->x));
}

int *returnFirst(int *a, int *b){ return a; } // pass in to funAiEngine's transformCoord when solving with real coords
// static inline void decrementFirst (int *f, int *s) { (*f)--; }
// static inline void decrementSecond(int *f, int *s) { (*s)--; }

void realSwap(GameVars *game, int swapy, int swapx, char direction) {
	swap0(game, swapy, swapx, direction);
	switch (statusGetch()) { 
		case 'c':
			longjmp(exitAi, 1);
		case 'q':
			endwin();
			exit(0);
	}
}

void transposeSwap(GameVars *game, int swapy, int swapx, char direction) {
	realSwap(game, swapx, swapy, direction);
}

// assumes starting to the right of the cell
void upRight(GameVars *game, SwapFunction swapf, int n) {
	while (n--) {
		swapf(game, 0, -1, 'r');
		swapf(game, 1, 0, 'd');
		swapf(game, 0, 1, 'l');
		if (n--) {
			swapf(game, -1, 0, 'u');
			swapf(game, 0, 1, 'l');
			swapf(game, 0, 1, 'd');
		}
	}
}

// all the positionFrom functions assume at least a 3x3 board
// different functions will be used for the top 2 cells in a column

// move 0 directly to the right of *a without
// without moving in to *a
// then move to the left in order to swap places with *a
void positionFromRight(GameVars *game, Coordinate *a, SwapFunction swapf) {
	// ensure 0 is not in same row as A before we move right
	//
	if (game->y == a->y) { // 0 in the same row as A
		if (game->x < a->x) { // 0 to the left of A
			if (game->y) { // there is a row above; move up to go around 
				swapf(game, -1, 0, 'd');
			}
			else { // we are at the top row; move down to go around
				swapf(game, 1, 0, 'u');
			}
		}
		else {
			// 0 in same row and to the right of a
			// just need to move to the left
			while (a->x < game->x) {
				swapf(game, 0, -1, 'r');
			}
			return;
		}
	}
	// 
	// 0 is now not in the same row as A

	// ensure 0 is to the right of A
	while (game->x <= a->x) {
		swapf(game, 0, 1, 'l');
	}

	// get 0 to the same row as A
	while (game->y < a->y) {
		swapf(game, 1, 0, 'u');
	}
	while (game->y > a->y) {
		swapf(game, -1, 0, 'd');
	}

	// move 0 in to *a from the right
	while (game->x > a->x) {
		swapf(game, 0, -1, 'r');
	}
}

// move 0 directly below *a without moving in to *a
// and without moving in to previously solved cells (anything below *b)
// then move up in order to swap places with *a
void positionFromBottom(GameVars *game, Coordinate *a, SwapFunction swapf) {
	if (game->x == a->x) {
		if (game->y < a->y) {
			// 0 is above *a
			// need to move to move around so we can come in from below
			if (game->x) { // if there is space to the left
				swapf(game, 0, -1, 'r');
			}
			else {
				swapf(game, 0, 1, 'l');
			}
		}
		else {
			// 0 is below *a
			// move up until we swap with it
			do {
				swapf(game, -1, 0, 'd');
			} while (game->y > a->y);
			return;
		}
	}
	//
	// 0 is now not in the same colum  as *a
	
	// ensure 0 is below *a
	while (game->y <= a->y) {
		swapf(game, 1, 0 , 'u');
	}

	// get 0 to row below *a
	while (game->y != a->y + 1) {
		swapf(game, -1, 0, 'd');
	}

	// get 0 to directly below *a
	while (game->x < a->x) {
		swapf(game, 0, 1, 'l');
	}
	while (game->x > a->x) {
		swapf(game, 0, -1, 'r');
	}

	// move up into *a
	swapf(game, -1, 0, 'd');
}

// happens in 2 phases:
// 	1. move 0 to the side of A it needs to be on
// 	2. move A diagonally until it's either in the same row or column as B
// 	3. Move A in a horizontal/vertical line to B if neccesary
void moveAToB(GameVars *game, Coordinate *a, Coordinate *b, int* (transformInts)(int*, int*), SwapFunction swapf) {
	transformInts(&(game->x), &(game->y)); // must undo at end of function lest the real coordinates will be transposed
	int dy, dx;
	const int vertDist = b->y - a->y;
	const int horDist = b->x - a->x;
	if (abs(horDist) > abs(vertDist)) {
		// get 0 directly to the right of A
		// then move left in to it
		positionFromRight(game, a, swapf);
	}
	else if (abs(horDist) < abs(vertDist)) {
		// vertical distance > horizontal distance
		// need to figure out if must come from above or below
		if (vertDist > 0) {
			positionFromBottom(game, a, swapf);
		}
		else {

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
		 *--------+XXXXXXXXXX
		 *XXXXXXXXXA!XXXXXXXX
		 *XXXXXXXXX!+--------
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
	GameVars *game, int row, int col,
	int* (*transformInts)(int*, int*),
	CoordGetter getCoord,
	SwapFunction swapf
) {
	for (int *i = transformInts(&row, &col); *i > 1; (*i)--) {
		Coordinate a;
		getCoord(game, row * game->cols + col, &a);
		Coordinate b = {row, col};
		transformInts(&(b.y), &(b.x));
		moveAToB(game, &a, &b, transformInts, swapf);
	} 
}

void funAi(GameVars *game) {
	// make array of cell coordinates
	const int length = game->rows * game->cols - 1;
	int coordinates[length];
	game->coordinates = coordinates;
	int i = 0;
	for (const int zeroCoord = game->y * game->cols + game->x; i < zeroCoord; i++) {
		const int cell = game->cells[i / game->cols][i % game->cols];
		game->coordinates[cell - 1] = i;
	}

	i++; // skip over 0
	for (; i < length; i++) {
		const int cell = game->cells[i / game->cols][i % game->cols];
		game->coordinates[cell - 1] = i;
	}

	// make it so there are as many unsolved rows as unsolved columns
	if (game->cols > game->rows) { // if more columns than rows
		int i = game->cols - game->rows;
		do {
			funAiEngine(game, game->rows - 1, game->rows - 1 + i, returnFirst, getRealCoord, realSwap);
		} while (--i);
	}
	else if (game->rows < game->cols) {

	}

	// the unsolved area is now a square
	// solve rows and columns one after another until we get to a 2x2
	// for (int i = 0; i < game->rows - 2; i++) {
	//         const int cornerIndex = game->rows - 1 - i;
	//         funAiEngine(game, cornerIndex, cornerIndex);
	//         funAiEngine(game, cornerIndex, cornerIndex - 1);
	// }

	// now solve the 2x2
}

void ai(GameVars *game) {
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
				funAi(game);
				return;
		}
	}
	clearMsgs();
	return;
}
