#include <ncurses.h>
#include <unistd.h>
#include <setjmp.h>
#include "drawing.h"
#include "undo.h"
#include "game_vars.h"
#include "test.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// quality of life macros for programming the ai
#define UP() swap(game, -1, 0, 'd')
#define DOWN() swap(game, 1, 0, 'u')
#define LEFT() swap(game, 0, -1, 'r')
#define RIGHT() swap(game, 0, 1, 'l')
#define DO360() \
do { \
	LEFT(); \
	UP(); \
	UP(); \
	RIGHT(); \
	RIGHT(); \
	DOWN(); \
	LEFT(); \
} while (false)
#define DOMOVES(moves) doMoves(game, swap, moves)

#define MOVE_DELAY_MS -1

typedef struct Coordinate{
	int y;
	int x;
} Coordinate;

typedef void (*SwapFunction)(GameVars*, int, int, char);
typedef void (*IntsTransform)(GameVars*, int*, int*);

typedef struct GridTransforms {
	void (*getCoord)(GameVars*, int, Coordinate*); 
	int *(*returnNth)(int*, int*);
	IntsTransform transformInts;
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

// int transforms
void doNothing(GameVars *game, int *y, int *x) {}
void negTransform(GameVars *game, int *y, int*x) {
	*y = game->rows - 1 - *y;
}
void swapInts(GameVars *game, int *y, int *x) {
	const int temp = *y;
	*y = *x;
	*x = temp;
}
void negTransposedTransform(GameVars *game, int *y, int *x) {
	swapInts(game, y, x);
	*y = game->rows - 1 - *y;
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
void getTransposedCoord(GameVars *game, int v, Coordinate *coord) {
	getRealCoord(game, v, coord);
	swapInts(game, &(coord->y), &(coord->x));
}

void getNegCoord(GameVars *game, int v, Coordinate *coord) {
	getRealCoord(game, v, coord);
	coord->y = (game->cols - 1) - coord->y;
}

// void getNegTransposedCoord(GameVars *game, int v, Coordinate *coord) {
//         getTransposedCoord(game, v, coord);
//         coord->y = (game->cols - 1) - coord->y;
// }

int *returnFirst(int *a, int *b){ return a; }
int *returnSecond(int *a, int *b){ return b; }

void realSwap(GameVars *game, int swapy, int swapx, char direction) {
	// update coordinate of swapped cell
	const int v = getV(game, game->y + swapy, game->x + swapx);
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

void negSwap(GameVars *game, int swapy, int swapx, char direction) {
	switch (direction) {
		case 'u':
			direction = 'd';
			break;
		case 'd':
			direction = 'u';
	}
	realSwap(game, -swapy, swapx, direction);
}

void transposedNegSwap(GameVars *game, int swapy, int swapx, char direction) {
	/* transposing followed by negating is described by this matrix:
	 * |0 -1|
	 * |1  0|
	 * with vectors being <y, x> 
	 * aka a 90 degree counterclockwise rotation
	 * we need to invert this matrix bc we need to convert
	 * transformed coords to real coords:
	 * the inverse is a 90 degree clockwise rotation:
	 * | 0 1|
	 * |-1 0*/
	switch (direction) {
		case 'u':
			direction = 'l';
			break;
		case 'l':
			direction = 'u';
			break;
		case 'd':
			direction = 'r';
			break;
		case 'r':
			direction = 'd';
	}
	const int temp = swapy;
	swapy = swapx;
	swapx = -temp;
	realSwap(game, swapy, swapx, direction);
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
// shifts cell to the right, moving above it
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
// shifts cell to the right, moving below it
void shiftRightD(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		DOWN();
		RIGHT();
		RIGHT();
		UP();
		LEFT();
	}
}

// assumes starting to the left of the cell
void shiftRightU(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		UP();
		RIGHT();
		RIGHT();
		DOWN();
		LEFT();
	}
}

// assumes starting above cell
void shiftDown(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		LEFT();
		DOWN();
		DOWN();
		RIGHT();
		UP();
	}
}

// assumes starting below cell
void shiftUp(GameVars *game, SwapFunction swap, int n) {
	while (n-- > 0) {
		LEFT();
		UP();
		UP();
		RIGHT();
		DOWN();
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
void positionFromRight(GameVars *game, Coordinate *a, int doneCol, GridTransforms *funcs) {
	interactiveDebug("fromRight");
	int transx, transy;
	transy = game->y;
	transx = game->x;
	funcs->transformInts(game, &transy, &transx);
	const SwapFunction swap = funcs->swap;

	if (transy == a->y) { // 0 in the same row as A
		if (transx < a->x) { // 0 to the left of A
			if (transy) { // there is a row above; move up to go around 
				UP();
				moveRightFor(game, swap, a->x - transx + 1);
				DOWN();

			}
			else { // we are at the top row; move down to go around
				DOWN();
				moveRightFor(game, swap, a->x - transx + 1);
				UP();
			}
			LEFT();
		}
		else {
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
	if (doneCol == a->x + 1 &&  a->y < transy) {
		if (transx == a->x) {
			LEFT();
			transx--;
		}
		moveUpFor(game, swap, transy - (a->y - 1));
		transy = a->y - 1;
	}
	moveRightFor(game, swap, a->x + 1 - transx);

	// ensure 0 is in the column directly to the right of *a
	moveLeftFor(game, swap, transx - (a->x + 1));

	// get 0 directly to the right of *a
	moveUpFor(game, swap, transy - a->y);
	moveDownFor(game, swap, a->y - transy);

	LEFT();
}

// special case when moving second to last cell
// in to pre-position
void positionFromRightSpecial(GameVars *game, Coordinate *a, int doneCol, GridTransforms *funcs) {
	// special case where second to last cell is 1 to the left of its pre position
	// this single special case is the only reason this function exists
	if (a->y == 0 && a->x == doneCol - 1) {
		int transx, transy;
		transy = game->y;
		transx = game->x;
		funcs->transformInts(game, &transy, &transx);
		const SwapFunction swap = funcs->swap;

		if (transy == a->y) {
			if (transx == a->x + 1) {
				LEFT();
				return;
			}
			DOWN();
			transy++;
		}
		// 0 is below *a

		moveUpFor(game, swap, transy - 1);
		moveRightFor(game, swap, doneCol - transx);
		UP();
		LEFT();
	}
	else {
		positionFromRight(game, a, doneCol, funcs);
	}
}

// move 0 directly below *a without moving in to *a
// and without moving in to previously solved cells (anything below *b)
// then move up in order to swap places with *a
void positionFromBottom(GameVars *game, Coordinate *a, GridTransforms *funcs) {
	interactiveDebug("fromBottom");
	int transx, transy;
	transy = game->y;
	transx = game->x;
	funcs->transformInts(game, &transy, &transx);
	const SwapFunction swap = funcs->swap;

	if (transx == a->x) {
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
	funcs->transformInts(game, &transy, &transx);
	const SwapFunction swap = funcs->swap;
	
	if (transx == a->x) {
		interactiveDebug("transx == a->x: %i", transx == a->x);
		// 0 is in same column as *a
		if (transy < a->y) {
			interactiveDebug("transy < a->y: %i", transy < a->y);
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
		interactiveDebug("returning");
		return;
	}
	
	// 0 is not in same column as *a
	
	// get 0 above *a
	moveUpFor(game, swap, transy - (a->y - 1));

	// get 0 to same column as *a
	moveLeftFor(game, swap, transx - a->x);
	moveRightFor(game, swap, a->x - transx);

	// get 0  directly above *a
	moveDownFor(game, swap, a->y - 1 - transy);

	DOWN();
}

// happens in 2 phases:
// 	1. move 0 to the side of A it needs to be on
// 	2. move A diagonally until it's either in the same row or column as B
// 	3. Move A in a horizontal/vertical line to B if neccesary
void moveAToB(GameVars *game, Coordinate *a, Coordinate *b, int doneCol, GridTransforms *funcs) {
	const int vertDist = b->y - a->y;
	const int horDist = b->x - a->x;
	if (!vertDist && !horDist) {
		return;
	}

	int transx = game->x;
	int transy = game->y;
	funcs->transformInts(game, &transy, &transx);
	const SwapFunction swap = funcs->swap;
	interactiveDebug("vertDist: %i", vertDist);
	if (vertDist >= 0) { // a* is above b*
		if (horDist > vertDist) { // more distance horizontally than vertically
			positionFromRight(game, a, doneCol, funcs);
			if (vertDist) {
				interactiveDebug("down right up");
				DOWN();
				RIGHT();
				UP();
			}
			downRight(game, swap, 2 * vertDist - 1);
			const int alreadyTraveled = 1 + vertDist; // positionFromRight moves it over by 1
			 					  // then downRight moves it over by vertDist
			shiftRightU(game, swap, b->x - a->x - alreadyTraveled);
		}
		else if (horDist < vertDist) { // more distance vertically than horizontally
			positionFromBottom(game, a, funcs);
			// (vertDist - 1)
			// because vertical distance changed by 1 in positionFromBottom
			downRight(game, swap, 2 * horDist);
			
			// interactiveDebug("vertDist - horDist: %i", vertDist - horDist);
			shiftDown(game, swap, vertDist  - 1 - horDist);
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
		else if ((a->x <= transx) && (a->y >= transy)) {
			// interactiveDebug("a->x <= game->x: %i, a->y >= game->y: %i", a->x <= game->x, a->y >= game->y)
			positionFromRight(game, a, doneCol, funcs);
			interactiveDebug("down right up");
			DOWN();
			RIGHT();
			UP();
			interactiveDebug("2 * (vertDist - 1): %i", 2 * (vertDist - 1));
			downRight(game, swap, 2 * (horDist - 1));
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
		// an important value is int vertness = a->y - b->y - (b->x - a->x)
		// there are 3 scenarios:
		// 1. vertness >= 1
		//	- positioning from the top is at minimum as efficient as from the right
		//	- if (horzDist > 1)
		//	  	1. right, up, left
		//	  	2. 2 * (horDist) - 3 cycles
		// 	- shift up a->y - b->y - 1 times
		// 	- 360
		// 2. vertness == 0
		//	- positioning from the top is at minimum as efficient as from the right
		//	- do 2 * (a->x - b->x - 1) cycles
		//	- 360
		// 3. vertness <= -1
		// 	- position from right
		// 	- (-2) * vertDistance - 1 cycles
		// 	- get to right of cell
		// 		- 360 if moving to the right would mess up solved cells
		//		- otherwise RIGHT(); UP(); LEFT();
		// 	- (-1 - vertness) right shifts

		const int vertness = a->y - b->y - (b->x - a->x);
		interactiveDebug("vertness: %i", vertness);
		if (vertness >= 1) {
			positionFromTop(game, a, funcs);
			if (horDist > 1) {
				RIGHT();
				UP();
				LEFT();
				upRight(game, swap, 2 * horDist - 3);
			}
			else if (vertDist > 1) {
				shiftUp(game, swap, -vertDist - horDist);
				DO360();
			}
		}
		else if (vertness == 0) {
			positionFromTop(game, a, funcs);
			upRight(game, swap, 2 * (a->x - b->x - 1));
			DO360();
		}
		else {
			positionFromRight(game, a, doneCol, funcs);
			interactiveDebug("wtf-2 * vertDist - 1: %i", -2 * vertDist - 1);
			upRight(game, swap, -2 * vertDist - 1);
			
			// when vertness < 0, the length of the box with corners *a and *b
			// is greater than the height of the box
			// the box also has a minimum height of 2
			// therefore the smallest box for scenario vertness < 0 is a 2 x 3
			// this is the only case where a 360 is neccesary to not mess up
			// solved cells. therefore check if horDist > 3 to see if we need
			// to 360
			const int minWidth = 3;
			if (horDist > minWidth) {
				RIGHT();
				UP();
				LEFT();
				shiftRightU(game, swap, -1 - vertness);
			}
			else {
				DO360();
			}
		}
	}
}

// performs a series of moves
void doMoves(GameVars *game, SwapFunction swap, char *moves) {
	do {
		switch (*moves) {
			case 'l':
				LEFT();
				break;
			case 'd':
				DOWN();
				break;
			case 'u':
				UP();
				break;
			case 'r':
				RIGHT();
		}
		moves++;
	} while (*moves);
}

// solve a column/tranposed column of the grid
// solve up to and including (col, row)
// the solve direction is determined by corresponding functions passed in
void funAiColumn(GameVars *game, GridTransforms *funcs, int row, int col) {
	// solve up to the top 2 cells in the column
	int *cellRow = funcs->returnNth(&row, &col);
	const int transcol = *funcs->returnNth(&col, &row);
	
	for (; *cellRow > 1; (*cellRow)--) {
		interactiveDebug("new cellRow");
		Coordinate a;
		funcs->getCoord(game, row * game->cols + col, &a);
		interactiveDebug("(a->y, a->x): (%i, %i)", a.y, a.x);
		Coordinate b = {row, col};
		funcs->transformInts(game, &b.y, &b.x);
		interactiveDebug("(b->y, b->x): (%i, %i)", b.y, b.x);
		moveAToB(game, &a, &b, transcol, funcs);
		mvhline(1, 0, ' ', 255);
	} 

	// solve the last 2 cells in the column
	// check if last 2 cells are already in position
	Coordinate secondLast, last;
	funcs->getCoord(game, game->cols * row + col, &secondLast);
	interactiveDebug("secondLast.y, secondLast.x: %i, %i", secondLast.y, secondLast.x);
	(*cellRow)--;
	funcs->getCoord(game, col, &last);

	const bool lastCorrect = last.y && (last.x == transcol);
	const bool secondLastCorrect = (secondLast.y == 1) && (secondLast.x == transcol);
	if (lastCorrect && secondLastCorrect) {
		return;
	}
		
	// there are a bunch of special cases when the last 2 cells are not in position
	// but are in the top right 2x2
	int transx = game->x;
	int transy = game->y;
	funcs->transformInts(game, &transy, &transx);
	const SwapFunction swap = funcs->swap;

	const bool lastInside = (last.y < 2) && (last.x >= transcol - 1);
	const bool secondLastInside = (secondLast.y < 2) && (secondLast.x >= transcol - 1);
	if (lastInside && secondLastInside) {
		interactiveDebug("special cases");
		// both are inside the 2x2 but not in position
		// now deal with a bunch of special cases
		// the solutions for the cases can be found with
		// $ python effic.py

		// 1 class of case for when 0 is in last col
		// the other for second to last col
		int transx = game->x;
		int temp = game->y;
		funcs->transformInts(game, &temp, &transx);

		const int lastIndex = last.y * game->cols + last.x;
		const int secondLastIndex = secondLast.y * game->cols + secondLast.x;
		// coordinates relative to top right 2x2
		const int zz = transcol - 1; // (0, 0)
		const int zo = zz + 1; // (0, 1)
		const int oz = zz + game->cols; // (1, 0)
		const int oo = oz + 1; // (1, 1)
		if (transx == transcol) {
			// 0 is in last col
			if (lastIndex == zz) {
				if (secondLastIndex == zo) {
					DOMOVES("ul");
				}
				else {
					DOMOVES("llurrdluldrrul");
				}
			}
			else if (lastIndex == zo) {
				if (secondLastIndex == zz) {
					DOMOVES("llurdrulldrurdl");
				}
				else {
					DOMOVES("l");
				}
			}
			else {
				if (secondLastIndex == zz) {
					DOMOVES("lurdl");
				}
				else {
					DOMOVES("ulldrurdllurdrul");
				}
			}
		}
		else {
			// 0 is in second to last col
			if (lastIndex == zz) {
				if (secondLastIndex == zo) {
					DOMOVES("lurrul");
				}
				else if (secondLastIndex == oz) {
					DOMOVES("lururdllurrdl");
				}
				else {
					DOMOVES("luurrdluldrrul");
				}
			}
			else if (lastIndex == zo) {
				if (secondLastIndex == zz) {
					DOMOVES("luurdrulldrurdl");
				}
				else {
					DOMOVES("lurruldlurrdl");
				}
			}
			else if (lastIndex == oz) {
				if (secondLastIndex == zz) {
					DOMOVES("urulddluurdrul");
				}
				else if (lastIndex == zo) {
					DOMOVES("luruldruldrrul");
				}
				else {
					DOMOVES("lururdllurdrul");
				}
			}
			else {
				if (secondLastIndex == zz) {
					DOMOVES("lurruldrul");
				}
				else if (secondLastIndex == zo) {
					DOMOVES("uruldlurrdluldrrul");
				}
				else {
					DOMOVES("luurrdl");
				}
			}
		}
		return;
	}

	// not both are inside the 2x2
	// we can move them normally
	int horDist = transcol - secondLast.x;
	int vertness = secondLast.y - horDist;
	interactiveDebug("horDist: %i, vertness: %i", horDist, vertness);
	// vertness >= 0:
	//	* positionFromTop
	//	* 2 * (b->x - a->x) cycle
	// 	* vertness upshifts
	// 	* RIGHT(); UP(); LEFT();
	// else:
	// 	* positionFromRight
	// 	* 2 * (a->y - b->y) cycles
	// 	* -vertness - 1 rightshifts
	if (vertness >= 0) {
		const bool secondLastInOneZero = secondLast.y == 1 && horDist == 1;
		const bool zeroInOneOne = last.y == 1 && last.x == transcol;
		if (secondLastInOneZero && zeroInOneOne) {
			/* special case:
			 * XA
			 * B0
			 * X can be anything, A is goal position, B is initial position 0 is 0
			 * In this case do LEFT(); UP(); RIGHT(); DOWN();
			 */
			LEFT();
			UP();
			RIGHT();
			DOWN();
		}
		else {
			positionFromTop(game, &secondLast, funcs);
			RIGHT();
			UP();
			LEFT();
			const bool wouldIntersect = vertness; // would intersect with solve cells
			if (wouldIntersect) {
				upRight(game, swap, 2 * (horDist - 1) - 1);
				shiftUp(game, swap, vertness);
				RIGHT();
				UP();
				LEFT();
			}
			else {
				upRight(game, swap, 2 * (horDist - 1));
			}
		}
	}
	else {
		// special case
		// if last is in its correct position and secondLast is not
		// when we try to move in secondLast we will get last stuck
		const int diagCycles = 2 * secondLast.y;
		const bool wouldStuck = last.y == 0 && last.x == transcol;
		if (wouldStuck) {
			// special case: vertness == -1
			// 1. move last so it's 2 to the left and 1 below its correct position
			// 2. move in to the last position from the right
			// 3. dllupdrul
			if (vertness == -1) {
				if (secondLast.y > 1) {
					positionFromRight(game, &secondLast, transcol, funcs);
					upRight(game, swap, 2 * (secondLast.y - 1) - 1);

					// position 0 to right of correct position
					RIGHT();
					UP();
					UP();
				}
				else {
					// move 0 to the right of correct position
					// without running over secondLast
					if (transx == secondLast.x) {
						RIGHT();
						moveUpFor(game, swap, transy);
					}
					else {
						moveUpFor(game, swap, transy);
						moveRightFor(game, swap, transx);
					}
				}

				// 0 is now in position
				DOMOVES("rdllurdrul");
			}
			else {
				positionFromRight(game, &secondLast, transcol, funcs);
				upRight(game, swap, diagCycles - 1);

				interactiveDebug("wouldStuck vertness < -1");
				// if we get any close than 2 without
				// preparation we'll get stuck so - 2
				// (horDist - 1) bc positionFromRight
				// moved it over 1
				const int shiftDist = (horDist - 1) - 2; 
				interactiveDebug("modified shiftright %i times", shiftDist);
				for (int i = 0; i < shiftDist; i++) {
					RIGHT();
					UP();
					LEFT();
					DOWN();
					RIGHT();
					RIGHT();
				}

				interactiveDebug("choreographed rrulldrrul");
				DOMOVES("rrulldrrul");
			}
		}
		else {
			positionFromRightSpecial(game, &secondLast, transcol, funcs);
			upRight(game, swap, diagCycles);
			shiftRightD(game, swap, horDist - 1 - secondLast.y);
		}
	}

	// secondLast is in pre-position
	// moving last to its preposition is not a special case
	// if we turn the board upside down
	
	// set the grid transform functions
	// returnNth and getCoord are irrelevant
	IntsTransform originalTransform = funcs->transformInts;
	if (funcs->transformInts == &doNothing) {
		interactiveDebug("normal board");
		funcs->transformInts = &negTransform;
		funcs->swap = &negSwap;
	}
	else {
		interactiveDebug("transposed board");
		// notice that transform is negTranposed but swap is transposedNeg
		// this is because T(N(<y, x>)) does not equal N(T(<y, x>)) in general
		// transform is NT because transform is meant to be used on real coordinates
		// and therefore we want to replicate the actual transformation
		// swap is TN because swap is done in transformed coordinates;
		// we need to work backwards to get to real coordinates
		funcs->transformInts = &negTransposedTransform;
		funcs->swap = &transposedNegSwap;
	}
	Coordinate finalPos = {row, col};
	funcs->transformInts(game, &finalPos.y, &finalPos.x);
	finalPos.x--;
	funcs->getCoord(game, row * game->cols + col, &last);
	negTransform(game, &last.y, &last.x);
	interactiveDebug("*cellRow: %i, transcol: %i, finalPos.y: %i, finalPos.x: %i", *cellRow, transcol, finalPos.y, finalPos.x);
	interactiveDebug("moving (transformed) (%i, %i) to (%i, %i)", last.y, last.x, finalPos.y, finalPos.x);
	moveAToB(game, &last, &finalPos, transcol, funcs);

	// both in pre-position; make the final rotation
	interactiveDebug("final rotation");
	transy = game->y;
	transx = game->x;
	originalTransform(game, &transy, &transx);
	interactiveDebug("transy: %i, transx: %i", transy, transx);
	if (!transy) {
		interactiveDebug("transy = %i > 0 so DOWN();RIGHT();", transy)
		DOWN();
		RIGHT();
	}
	RIGHT();
	UP();
	LEFT();
}

void funAi(GameVars *game) {
	// make array of cell coordinates
	const int length = game->rows * game->cols - 1;
	int coordinates[length];
	game->coordinates = coordinates;
	int i = 0;
	for (const int zeroCoord = game->y * game->cols + game->x; i < zeroCoord; i++) {
		const int cell = getV(game, i / game->cols, i % game->cols);
		game->coordinates[cell - 1] = i;
		// interactiveDebug("%i", cell);
	}
	i++; // skip over 0
	for (; i < length + 1; i++) {
		const int cell = getV(game, i / game->cols, i % game->cols);
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
