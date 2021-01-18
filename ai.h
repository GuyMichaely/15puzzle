#include <ncurses.h>
#include <unistd.h>
#include <setjmp.h>

#include "drawing.h"
#include "undo.h"
#include "swap_ints.h"
#include "game_vars.h"
#include "test.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// quality of life macros for programming the ai
#define UP() swap(game, -1, 0, 'd')
#define DOWN() swap(game, 1, 0, 'u')
#define LEFT() swap(game, 0, -1, 'r')
#define RIGHT() swap(game, 0, 1, 'l')

#define MOVE_DELAY_MS -1

typedef struct Coordinate{
	int y;
	int x;
} Coordinate;

typedef void (*SwapFunction)(GameVars*, int, int, char);

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


void realSwap(GameVars *game, int swapy, int swapx, char direction) {
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
			mvhline(0, 0, ' ', 255);
	}
}

void transposedSwap(GameVars *game, int swapy, int swapx, char direction) {
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
	realSwap(game, swapx, swapy, direction);
}

// assumes starting to the left of the cell
void upRight(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		UP();
		RIGHT();
		DOWN();
		if (n-- > 0) {
			RIGHT();
			UP();
			LEFT();
		}
	}
}

// assumes starting above the cell
void downRight(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		RIGHT();
		DOWN();
		LEFT();
		if (n-- > 0) {
			DOWN();
			RIGHT();
			UP();
		}
	}
}

// assumes starting to the left of the cell
void shiftRightD(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		UP();
		RIGHT();
		RIGHT();
		DOWN();
		LEFT();
	}
}

// assumes starting to the left of the cell
void shiftRightU(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		DOWN();
		RIGHT();
		RIGHT();
		UP();
		LEFT();
	}
}

// assumes starting below cell
void shiftDown(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		LEFT();
		UP();
		UP();
		RIGHT();
		DOWN();
	}
}

// assumes starting above cell
void shiftUp(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		LEFT();
		DOWN();
		DOWN();
		RIGHT();
		UP();
	}
}

void moveLeftFor(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		LEFT();
	}
}

void moveRightFor(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		RIGHT();
	}
}

void moveDownFor(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		DOWN();
	}
}

void moveUpFor(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		UP();
	}
}

// all the positionFrom functions assume at least a 3x3 board
// different functions will be used for the top 2 cells in a column

// move 0 directly to the right of *a without
// without moving in to *a
// then move to the left in order to swap places with *a
void positionFromRight(GameVars *game, Coordinate *a, GridTransforms *funcs) {
	interactiveDebug("fromRight");
	int transx, transy;
	transy = game->y;
	transx = game->x;
	funcs->transformInts(&transy, &transy);
	const SwapFunction swap = funcs->swap;

	if (transy == a->y) { // 0 in the same row as A
		interactiveDebug("transy == a->y");
		if (transx < a->x) { // 0 to the left of A
			interactiveDebug("transx < a->x");
			if (transy) { // there is a row above; move up to go around 
				interactiveDebug("transy");
				UP();
				moveRightFor(game, swap, a->x - transx + 1);
				DOWN();

			}
			else { // we are at the top row; move down to go around
				interactiveDebug("!transy");
				DOWN();
				moveRightFor(game, swap, a->x - transx + 1);
				UP();
			}
			LEFT();
		}
		else {
			interactiveDebug("transx>= a->x");
			// 0 in same row and to the right of *a
			// just need to move to the left
			for (int i = transx - a->x; i; i--) {
				LEFT();
			}
		}
		return;
	}

	// 0 is not in the same row as *a

	// ensure 0 is to the right of *a
	moveRightFor(game, swap, a->x + 1 - transx);

	// ensure 0 is in the column directly to the right of *a
	moveLeftFor(game, swap, transx - (a->x + 1));

	// get 0 directly to the right of *a
	moveUpFor(game, swap, transy - a->y);
	moveDownFor(game, swap, a->y - transy);

	LEFT();
}

// move 0 directly below *a without moving in to *a
// and without moving in to previously solved cells (anything below *b)
// then move up in order to swap places with *a
void positionFromBottom(GameVars *game, Coordinate *a, GridTransforms *funcs) {
	interactiveDebug("fromBottom");
	int transx, transy;
	transy = game->y;
	transx = game->x;
	funcs->transformInts(&transy, &transy);
	const SwapFunction swap = funcs->swap;

	if (transx == a->x) {
		interactiveDebug("transx == a->x");
		if (transy < a->y) {
			interactiveDebug("transy < a->y");
			// 0 is above *a
			// need to move to move around so we can come in from below
			if (transx) { // if there is space to the left
				interactiveDebug("transx");
				LEFT();
				moveDownFor(game, swap, a->y + 1 - transy);
				RIGHT();
			}
			else {
				interactiveDebug("!transx");
				RIGHT();
				moveDownFor(game, swap, a->y + 1 - transy);
				LEFT();
			}
			UP();
		}
		else {
			interactiveDebug("transy >= a->y");
			// 0 is below *a
			// move up until we swap with it
			moveUpFor(game, swap, transy - a->y);
		}
		return;
	}
	
	// 0 is not in same column as a*
	
	// ensure 0 is below *a
	moveDownFor(game, swap, a->y + 1 - transy);

	// get 0 to row below *a
	moveUpFor(game, swap, transy - (a->y + 1));

	// get 0 to directly below *a
	moveLeftFor(game, swap, transx - a->x);
	moveRightFor(game, swap, a->x - transx);
	
	// move up into *a
	UP();
}

// move 0 directly above *a without moving in to *a
// and without moving in to previously solved cells (anything below *b)
// then move down in order to swap places with *a
void positionFromTop(GameVars *game, Coordinate *a, GridTransforms *funcs) {
	interactiveDebug("fromTop");
	int transx, transy;
	transy = game->y;
	transx = game->x;
	funcs->transformInts(&transy, &transy);
	const SwapFunction swap = funcs->swap;
	
	if (transx == a->x) {
		// 0 is in same column as *a
		if (transy < a->y) {
			// 0 is in same column and above *a
			// just need to move down past *a
			moveDownFor(game, swap, a->y - transy);
		}
		else {
			// 0 is in same column and below *a
			// need to move to adjacent column to go around
			if (transx) { // if room on left
				LEFT();
				moveUpFor(game, swap, transy - (a->y - 1));
				RIGHT();
			}
			else {
				RIGHT();
				moveUpFor(game, swap, transy - (a->y - 1));
				LEFT();
			}
			DOWN();
		}
		return;
	}
	
	// 0 is not in same column as *a
	
	// get 0 above *a
	moveUpFor(game, swap, transy - (game->y - 1));

	// get 0 to row above *a
	moveDownFor(game, swap, (game->y - 1) - transy);

	// get 0 to cell above *a
	moveLeftFor(game, swap, transx - a->x);
	moveRightFor(game, swap, a->x - transx);

	DOWN();
}

// happens in 2 phases:
// 	1. move 0 to the side of A it needs to be on
// 	2. move A diagonally until it's either in the same row or column as B
// 	3. Move A in a horizontal/vertical line to B if neccesary
void moveAToB(GameVars *game, Coordinate *a, Coordinate *b, GridTransforms *funcs) {
	const int vertDist = b->y - a->y;
	const int horDist = b->x - a->x;
	const SwapFunction swap = funcs->swap;
	interactiveDebug("vertDist: %i", vertDist);
	if (vertDist >= 0) { // a* is above b*
		if (horDist > vertDist) { // more distance horizontally than vertically
			positionFromRight(game, a, funcs);
			downRight(game, swap, 2 * vertDist);
			shiftRightU(game, swap, b->x - (a->x + 1));
		}
		else if (horDist < vertDist) { // more distance vertically than horizontally
			positionFromBottom(game, a, funcs);
			// (vertDist - 1)
			// because vertical distance changed by 1 in positionFromBottom
			interactiveDebug("2 * (vertDist - 1): %i", 2 * (vertDist - 1));
			downRight(game, swap, 2 * (vertDist - 1));
			
			shiftDown(game, swap, b->x - a->x - (vertDist - 1));
		}

		// vertical and horizontal distances are equal
		// where you must go is determined by which side is closer
		//
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
		 */
		// if 0 in top right quadrant positionFromRight
		// otherwise positionFromBottom because it is either correct or irrelevant
		else if (a->x <= game->x && a->y <= game->y) {
			positionFromRight(game, a, funcs);
			interactiveDebug("2 * vertDist - 1: %i", 2 * vertDist - 1);
			shiftRightD(game, swap, 2 * vertDist - 1);
		}
		else {
			positionFromBottom(game, a, funcs);
			downRight(game, swap, 2 * vertDist - 1);
		}
	}
	else {
		// A is below B
		// this is a pain because we aren't allowed to enter any cells below B
		// because they're already been set correctly
		
		// an important value is int k = b->x - b->y - a->x - a->y
		// there are 4 scenarios:
		// 0. k is wrong
		// 	- 
		// 1. k > 1
		// 	- positioning from the right as at minimum as efficient as from above
		// 	- do 2*(a->y - b->y) cycles
		// 	- then shift right b->x - a->x - 2 cycles
		// 2. k == 1
		//	- positioning from the top is at minimum as efficient as from the right
		//	- do 1 + 2 * (a->y - b->y - 1) cycles
		//	- then shift right
		// 3. k <= 0
		//	- positioning from the top is at minimum as efficient as from the right
		//	- do 2 * (a->x - b->x - 1) cycles
		//	- do a->y - b->y - 2 upshifts
		//	- then move down and 360

		const int k = b->x - b->y - a->x - a->y;
		if (k > 1) {
			positionFromRight(game, a, funcs);
			upRight(game, swap, 2 * (a->y - b->y));
			shiftRightU(game, swap, b->x - a->x - 2);
		}
		else if (k == 1) {
			positionFromTop(game, a, funcs);
			RIGHT();
			DOWN();
			LEFT();
			upRight(game, swap, 2 * (a->y - b->y - 1));
			shiftRightU(game, swap, 1);
		}
		else {
			positionFromTop(game, a, funcs);
			upRight(game, swap, 2 * (a->x - b->x - 1));
			shiftUp(game, swap, 2);
			// "360"
			LEFT();
			UP();
			UP();
			RIGHT();
			RIGHT();
			DOWN();
			LEFT();
		}
	}







        //
	// if (abs(horDist) > abs(vertDist)) {
	//         // get 0 directly to the right of A
	//         // then move left in to it
	//         positionFunction = &positionFromRight;
	// }
	// else if (abs(horDist) < abs(vertDist)) {
	//         // vertical distance > horizontal distance
	//         // need to figure out if must come from above or below
	//         if (vertDist > 0) {
	//                 positionFunction = &positionFromBottom;
	//         }
	//         else {
	//                 positionFunction = &positionFromTop;
	//         }
	// }
	// else {
	//         if (vertDist > 0) {
	//                 // corresponds to situation in diagram
	//                 // if 0 in upper right quadrant send to right of a*
	//                 // otherwise send below *a
	//                 funcs->transformInts(&game->y, &game->x);
	//                 if (a->x <= game->x && game->y <= a->y) {
	//                         positionFunction = &positionFromRight;
	//                 }
	//                 else {
	//                         positionFunction = &positionFromBottom;
	//                 }
	//                 funcs->transformInts(&game->y, &game->x);
	//         }
	//         else {
	//                 // if 0 in bottom right quadrant send to right of a*
	//                 // otherwise send above *a
	//                 funcs->transformInts(&game->y, &game->x);
	//                 if (a->x <= game->x && a->y <= game->y) {
	//                         positionFunction = &positionFromRight;
	//                 }
	//                 else {
	//                         positionFunction = &positionFromTop;
	//                 }
	//                 funcs->transformInts(&game->y, &game->x);
	//         }
	// }
	// positionFunction(game, a, funcs);
        //
	// // now move tile to final destination
	// if (positionFunction == &positionFromRight) {
	//         if (vertDist > 0) {
	//                 // use upRight
	//                 // because we're using psotionFromRight we know there's at least
	//                 // as much space on the right as the top
	//                 // meaning we can't go out of bounds on the right
	//                 // we can make an illegal move if *a and *b form a square
        //
	//                 const int cycles= 2 * MIN(vertDist, horDist) - (vertDist == horDist);
	//                 upRight(game, &transformedCoord, funcs->swap, cycles);
	//
	//         }
	//         else {
	//                 // use downRight
	//                 // similar logic for upRight but upside down
	//                 // no illegal moves here only out of bounds
	//                 const int cycles = 2 * MIN(-vertDist, horDist);
	//                 downRight(game, &transformedCoord, funcs->swap, cycles);
	//                 shiftRightU(game, &transformedcoord, funcs->swap, b->x - a->x);
	//         }
	// }
	// else if (positionFunction == &positionFromBottom) {
	//
	// }
	// else { // positionFromTop
	//
	// }
}

// solve a column/tranposed column of the grid
// solve up to and including (col, row)
// the solve direction is determined by corresponding functions passed in
void funAiColumn(GameVars *game, GridTransforms *funcs, int row, int col) {
	printArr(1, 0, game->coordinates, game->rows * game->cols - 1);
	statusGetch();
	mvhline(1, 0, ' ', 255);
	for (int *i = funcs->returnNth(&row, &col); *i > 1; (*i)--) {
		interactiveDebug("new cell");
		Coordinate a;
		funcs->getCoord(game, row * game->cols + col, &a);
		interactiveDebug("(a->y, a->x): (%i, %i)", a.y, a.x);
		Coordinate b = {row, col};
		funcs->transformInts(&(b.y), &(b.x));
		moveAToB(game, &a, &b, funcs);
		printArr(1, 0, game->coordinates, game->rows * game->cols - 1);
		statusGetch();
		mvhline(1, 0, ' ', 255);
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
		// interactiveDebug("%i", cell);
	}
	i++; // skip over 0
	for (; i < length + 1; i++) {
		const int cell = game->cells[i / game->cols][i % game->cols];
		game->coordinates[cell - 1] = i;
		// interactiveDebug("%i", cell);
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
		interactiveDebug("transposing");
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
