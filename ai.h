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

typedef void (*SwapFunction)(GameVars*, Coordinate*, int, int, char);

typedef struct GridTransforms {
	void (*getCoord)(GameVars*, int, Coordinate*); 
	int *(*returnNth)(int*, int*);
	void (*transformInts)(int*, int*);
	SwapFunction swap;
} GridTransforms;

jmp_buf exitAi; // buffer used for exiting ai when user hits 'c'

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
		case 'p':
		case 'P':
			return 'p';
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
	interactiveDebug("finding %i", v);
	coord->y = game->coordinates[v - 1] / game->cols;
	coord->x = game->coordinates[v - 1] % game->cols;
}

// stores coordinate of v in *coord
void getTransposedCoord(GameVars *game, int v, Coordinate *coord) {
	getRealCoord(game, v, coord);
	swapInts(&(coord->y), &(coord->x));
}

int *returnFirst(int *a, int *b){ return a; }
int *returnSecond(int *a, int *b){ return b; }

void realSwap(GameVars *game, Coordinate *transformedCoord, int swapy, int swapx, char direction) {
	// update coordinate of swapped cell
	const int v = game->cells[game->y + swapy][game->x + swapx];
	int newCoordinate = game->coordinates[v - 1];
	newCoordinate -= game->cols * swapy + swapx;
	game->coordinates[v - 1] = newCoordinate;

	// swap the cells on screen and in game->cells
	swap0(game, swapy, swapx, direction);
	switch (statusGetch()) { 
		case 'c':
			longjmp(exitAi, 1);
		case 'q':
			endwin();
			exit(0);
		case 'p':
			printArr(0, 0, game->coordinates, game->rows * game->cols - 1);
			quitGetch();
	}

	transformedCoord->y = game->y;
	transformedCoord->x = game->x;
}

void transposedSwap(GameVars *game, Coordinate *transformedCoord, int swapy, int swapx, char direction) {
	switch (direction) {
		case 'r':
			direction = 'd';
			break;
		case 'u':
			direction = 'l';
			break;
		case 'l':
			direction = 'u';
			break;
		case 'd':
			direction = 'r';
	}
	realSwap(game, transformedCoord, swapx, swapy, direction);
	swapInts(&transformedCoord->y, &transformedCoord->x);
}

// assumes starting to the left of the cell
// void upRight(GameVars *game, SwapFunction swap, int n) {
//         while (n--) {
//                 swap(game, 0, -1, 'r');
//                 swap(game, 1, 0, 'd');
//                 swap(game, 0, 1, 'l');
//                 if (n--) {
//                         swap(game, -1, 0, 'u');
//                         swap(game, 0, 1, 'l');
//                         swap(game, 0, 1, 'd');
//                 }
//         }
// }

// all the positionFrom functions assume at least a 3x3 board
// different functions will be used for the top 2 cells in a column

// move 0 directly to the right of *a without
// without moving in to *a
// then move to the left in order to swap places with *a
void positionFromRight(GameVars *game, Coordinate *a, GridTransforms *funcs) {
	interactiveDebug("fromRight");
	Coordinate transformedCoord = {game->y, game->x};
	funcs->transformInts(&transformedCoord.y, &transformedCoord.x);
	const SwapFunction swap = funcs->swap;

	// ensure 0 is not in same row as A before we move right
	//
	if (transformedCoord.y == a->y) { // 0 in the same row as A
		if (transformedCoord.x < a->x) { // 0 to the left of A
			if (transformedCoord.y) { // there is a row above; move up to go around 
				swap(game, &transformedCoord, -1, 0, 'd');
			}
			else { // we are at the top row; move down to go around
				swap(game, &transformedCoord, 1, 0, 'u');
			}
		}
		else {
			// 0 in same row and to the right of a
			// just need to move to the left
			while (a->x < transformedCoord.x) {
				swap(game, &transformedCoord, 0, -1, 'r');
			}
			return;
		}
	}
	// 
	// 0 is now not in the same row as A

	// ensure 0 is to the right of A
	while (transformedCoord.x <= a->x) {
		swap(game, &transformedCoord, 0, 1, 'l');
	}

	// get 0 to the same row as A
	while (transformedCoord.y < a->y) {
		swap(game, &transformedCoord, 1, 0, 'u');
	}
	while (transformedCoord.y > a->y) {
		swap(game, &transformedCoord, -1, 0, 'd');
	}

	// move 0 in to *a from the right
	do {
		swap(game, &transformedCoord, 0, -1, 'r');
	} while (transformedCoord.x > a->x);
}

// move 0 directly below *a without moving in to *a
// and without moving in to previously solved cells (anything below *b)
// then move up in order to swap places with *a
void positionFromBottom(GameVars *game, Coordinate *a, GridTransforms *funcs) {
	interactiveDebug("fromBottom");
	Coordinate transformedCoord = {game->y, game->x};
	funcs->transformInts(&transformedCoord.y, &transformedCoord.x);
	const SwapFunction swap = funcs->swap;

	if (transformedCoord.x == a->x) {
		interactiveDebug("transformedCoord.x == a->x");
		if (transformedCoord.y < a->y) {
			interactiveDebug("transformedCoord.y < a->y");
			// 0 is above *a
			// need to move to move around so we can come in from below
			if (transformedCoord.x) { // if there is space to the left
				interactiveDebug("transformedCoord.x");
				swap(game, &transformedCoord, 0, -1, 'r');
			}
			else {
				interactiveDebug("!transformedCoord.x");
				swap(game, &transformedCoord, 0, 1, 'l');
			}
		}
		else {
			interactiveDebug("transformedCoord.y >= a->y");
			// 0 is below *a
			// move up until we swap with it
			do {
				swap(game, &transformedCoord, -1, 0, 'd');
			} while (transformedCoord.y > a->y);
			return;
		}
	}
	//
	// 0 is now not in the same colum  as *a
	
	// ensure 0 is below *a
	while (transformedCoord.y <= a->y) {
		interactiveDebug("transformedCoord.y <= a->y");
		swap(game, &transformedCoord, 1, 0 , 'u');
	}

	// get 0 to row below *a
	while (transformedCoord.y != a->y + 1) {
		interactiveDebug("transformedCoord.y != a->y + 1");
		swap(game, &transformedCoord, -1, 0, 'd');
	}

	// get 0 to directly below *a
	while (transformedCoord.x < a->x) {
		interactiveDebug("transformedCoord.x < a->x");
		swap(game, &transformedCoord, 0, 1, 'l');
	}
	while (transformedCoord.x > a->x) {
		interactiveDebug("transformedCoord.x > a->x");
		swap(game, &transformedCoord, 0, -1, 'r');
	}

	// move up into *a
	swap(game, &transformedCoord, -1, 0, 'd');
}

// move 0 directly above *a without moving in to *a
// and without moving in to previously solved cells (anything below *b)
// then move down in order to swap places with *a
void positionFromTop(GameVars *game, Coordinate *a, GridTransforms *funcs) {
	Coordinate transformedCoord = {game->y, game->x};
	funcs->transformInts(&transformedCoord.y, &transformedCoord.x);
	const SwapFunction swap = funcs->swap;
	
	if (transformedCoord.x == a->x) {
		// 0 is in same column as *a
		if (transformedCoord.y < a->y) {
			// 0 is in same column and above *a
			// just need to move down past *a
			do {
				swap(game, &transformedCoord, 0, 1, 'u');
			} while (transformedCoord.y < a->y);
			return;
		}
		else {
			// 0 is in same column and below *a
			// need to move to adjacent column to go around
			if (transformedCoord.x) { // if room on left
				swap(game, &transformedCoord, 0, -1, 'r');
			}
			else {
				swap(game, &transformedCoord, 0, 1, 'l');
			}
		}
	}
	//
	// 0 is now not in same column as *a
	
	// get 0 above *a
	while (transformedCoord.y > a->y) {
		swap(game, &transformedCoord, -1, 0, 'd');
	}

	// get 0 to same column as *a
	while (transformedCoord.x < a->x) {
		swap(game, &transformedCoord, 0, 1, 'l');
	}
	while (transformedCoord.x > a->x) {
		swap(game, &transformedCoord, 0, -1, 'r');
	}

	// 0 is now in same column and above *a; move down
	do {
		swap(game, &transformedCoord, 1, 0, 'u');
	} while (transformedCoord.y < a->y);
}

// happens in 2 phases:
// 	1. move 0 to the side of A it needs to be on
// 	2. move A diagonally until it's either in the same row or column as B
// 	3. Move A in a horizontal/vertical line to B if neccesary
void moveAToB(GameVars *game, Coordinate *a, Coordinate *b, GridTransforms *funcs) {
	const int vertDist = b->y - a->y;
	const int horDist = b->x - a->x;
	void (*positionFunction)(GameVars*, Coordinate*, GridTransforms*);
	if (abs(horDist) > abs(vertDist)) {
		// get 0 directly to the right of A
		// then move left in to it
		positionFunction = positionFromRight;
	}
	else if (abs(horDist) < abs(vertDist)) {
		// vertical distance > horizontal distance
		// need to figure out if must come from above or below
		if (vertDist > 0) {
			positionFunction = positionFromBottom;
		}
		else {
			positionFunction = positionFromTop;
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
		if (vertDist > 0) {
			// corresponds to situation in diagram
			// if 0 in upper right quadrant send to right of a*
			// otherwise send below *a
			funcs->transformInts(&game->y, &game->x);
			if (a->x <= game->x && game->y <= a->y) {
				positionFunction = positionFromRight;
			}
			else {
				positionFunction = positionFromBottom;
			}
			funcs->transformInts(&game->y, &game->x);
		}
		else {
			// if 0 in bottom right quadrant send to right of a*
			// otherwise send above *a
			funcs->transformInts(&game->y, &game->x);
			if (a->x <= game->x && a->y <= game->y) {
				positionFunction = positionFromRight;
			}
			else {
				positionFunction = positionFromTop;
			}
			funcs->transformInts(&game->y, &game->x);
		}
	}
	positionFunction(game, a, funcs);
}

// solve a column/tranposed column of the grid
// solve up to and including (col, row)
// the solve direction is determined by corresponding functions passed in
void funAiColumn(GameVars *game, GridTransforms *funcs, int row, int col) {
	printArr(1, 0, game->coordinates, game->rows * game->cols - 1);
	// interactiveDebug("New column");
	mvhline(1, 0, ' ', 255);
	for (int *i = funcs->returnNth(&row, &col); *i > 1; (*i)--) {
		Coordinate a;
		funcs->getCoord(game, row * game->cols + col, &a);
		Coordinate b = {row, col};
		funcs->transformInts(&(b.y), &(b.x));
		// interactiveDebug("a coords: (%i, %i) b coords (y, x): (%i, %i)", a.y, a.x, b.y, b.x);
		moveAToB(game, &a, &b, funcs);
		printArr(0, 0, game->coordinates, game->rows * game->cols - 1);
		quitGetch();
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
	i = game->cols - game->rows;
	if (i > 0) { // if more columns than rows
		GridTransforms funcs = {
			&getRealCoord,
			&returnFirst,
			&doNothing,
			&realSwap
		};
		do {
			funAiColumn(game, &funcs, game->rows - 1, game->rows - 1 + i);
		} while (--i);
	}
	else if (i < 0) {
		GridTransforms funcs = {
			&getTransposedCoord,
			&returnSecond,
			&swapInts,
			&transposedSwap
		};
		do {
			funAiColumn(game, &funcs, game->rows - 1 - i, game-> rows - 1);
		} while (++i);
	}

	// the unsolved area is now a square
	// solve rows and columns one after another until we get to a 2x2
	for (int i = 0; i < game->rows - 2; i++) {
		const int cornerindex = game->rows - 1 - i;
		GridTransforms funcs = {
			&getRealCoord,
			&returnFirst,
			&doNothing,
			&realSwap
		};
		funAiColumn(game, &funcs, cornerindex, cornerindex);
		
		funcs.getCoord = &getTransposedCoord;
		funcs.returnNth = &returnSecond;
		funcs.transformInts = &swapInts;
		funcs.swap = &transposedSwap;
		funAiColumn(game, &funcs, cornerindex, cornerindex - 1);
	}

	// now solve the 2x2
}

void ai(GameVars *game) {
	// jumped to by transposedSwap and realSwap when user hits 'c'
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
