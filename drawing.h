#pragma once

#include <ncurses.h>
#include <stdlib.h>
// #include "test.h"

#include "undo.h"

#include "game_vars.h"

inline static void midPrint(int row, char *string) {
	mvprintw(row, (COLS - strlen((string))) / 2, string);
}

// d is the length of the dimension
// margin length and grid length will be stored in *margin, *length
static inline int getMargin(int d){
	const int modLimit = 1; // if d % 6 > 1 margin has enough room to be 1 bigger
	return d / 6 + (d % 6 > modLimit);
}

// given the length of the dimension and number of cells to split it into
// stores line coordinates (relative to edge of box, not edge of screen)_in lineCoords
// returns length of last cell
void getCoords(int length, int split, int lineCoords[], int *cellCoords) {
	// most likely won't be able to divide the grid perfectly evenly
	// therefore spread the larger ones out throughtout the dimension
	// symmetrically and evenly

	// want to calculate seperators as though the lines don't take up space
	// will account for the difference when storing the data
	length -= (split - 1);

	// if there is an odd number of lines set the middle one
	if (!(split % 2)) {
		lineCoords[split / 2 - 1] = length / 2 + (split - 1) / 2; // (+ (split - 1) / 2 to account for the space the lines take
	}

	// iterate through gridlines out to in
	// set each gridline symmetrically on the left and right of the center
	const int lastIndex = (split + 1) / 2 - 1;
	for (int i = 0; i < lastIndex; i++) {
		lineCoords[i] = length * (i + 1) / split + i;
		lineCoords[(split - 2) - i] = length - lineCoords[i] + ((split - 1) - 1); // + (split - 1) to get length back to where it was, - 1 because last line is always too short
	}

	// if there's an odd number of lines
	// it can happen that the (n / 2 + 1)th section is too large
	// check and account for that
	if (split != 2 && !(split % 2)) {
		const int middle = split / 2 - 1;
		if ((lineCoords[middle + 1] - lineCoords[middle]) - (lineCoords[middle + 2] - lineCoords[middle + 1]) > 1) {
			lineCoords[middle + 1]--;
		}
	}

	// calculate cell coordinates
	length += (split - 1);

	cellCoords[0] = (lineCoords[0]) / 2;
	cellCoords[split - 1] = (length + lineCoords[split - 2]) / 2;
	for (int i = 1; i < (split + 1) / 2; i++) {
		cellCoords[split - 1 - i] = (lineCoords[split - 1 - i] + lineCoords[split - 2 - i]) / 2;
		cellCoords[i] = (lineCoords[i] + lineCoords[i - 1] + 1) / 2;
	}
}

// only for nonnegative integers
int intLength(int x) {
	int length = 1;
	for (int lt = 9; lt < x; lt = lt * 10 + 9) {
		length++;
	}
	return length;
}

// draws number at position, properly centering it
void drawNum(const int y, const int x, const int num, const bool onRight) {
	mvprintw(y, x - (intLength(num) - onRight) / 2, "%i", num);
}

// clears out cell at coordinate game->y, game->x
void clearSpot(const int yCoord, int xCoord, const int value, const bool onRight) {
	const int length = intLength(value);
	xCoord -= (length - onRight) / 2;
	mvhline(yCoord, xCoord, ' ', length);
}

// swap 0 cell with (swapy, swapx)
// do not add move to the undo list
void swap0NoUndo(GameVars *game, int swapy, int swapx) {
	// the swaps are relative to coordinate of 0
	swapy += game->y;
	swapx += game->x;

	const bool xOnRight     =     game->x >= (game->cols + 1) / 2;
	const bool swapXOnRight = swapx >= (game->cols + 1) / 2;

	clearSpot(game->yCoords[game->y], game->xCoords[game->x], 0, xOnRight);
	drawNum(game->yCoords[game->y], game->xCoords[game->x], game->cells[swapy][swapx], xOnRight);

	clearSpot(game->yCoords[swapy], game->xCoords[swapx], game->cells[swapy][swapx], swapXOnRight);
	attron(A_BOLD);
	drawNum(game->yCoords[swapy], game->xCoords[swapx], 0, swapXOnRight);
	attroff(A_BOLD);
	game->cells[game->y][game->x] = game->cells[swapy][swapx];
	
	game->x = swapx;
	game->y = swapy;
}

// swap 0 cell with (swapy, swapx)
// add the swap to the undo list
void swap0(GameVars *game, const int swapy, const int swapx, Move **undo, const char undoDirection) {
	swap0NoUndo(game, swapy, swapx);


	// add move to undo list
	Move* newMove = malloc(sizeof(Move));
	newMove->move = undoDirection;
	newMove->prev = *undo;
	*undo = newMove;

	refresh();
}

// executes f on every cell
// skips (game->y, game->x) and executes f on that cell last
void cellsMap(GameVars *game, void (*f)(int, int, int, bool)) {
	// iterate up to game->y row
	for (int y = 0; y < game->y; y++) {
		for (int x = 0; x < (game->cols + 1) / 2; x++) {
			f(game->yCoords[y], game->xCoords[x], game->cells[y][x], false);
		}
		for (int x = (game->cols + 1) / 2; x < game->cols; x++) {
			f(game->yCoords[y], game->xCoords[x], game->cells[y][x], true);
		}
	}

	// iterate up to either halfway or game->x
	int x = 0;
	for (; x < game->x && x < (game->cols + 1) / 2; x++) {
		f(game->yCoords[game->y], game->xCoords[x], game->cells[game->y][x], false);
	}

	// if stopped because reached game->x, iterate until halfway
	// else if stopped because reached halfway, continue until reach game->x
	// then continue
	if (x == game->x) {
		x++;
		for (; x < (game->cols + 1) / 2; x++) {
			f(game->yCoords[game->y], game->xCoords[x], game->cells[game->y][x], false);
		}
	}
	else {
		for (; x < game->x; x++) {
			f(game->yCoords[game->y], game->xCoords[x], game->cells[game->y][x], true);
		}
		x++;
	}
	for (; x < game->cols; x++) {
		f(game->yCoords[game->y], game->xCoords[x], game->cells[game->y][x], true);
	}

	// iterate through the rest
	for (int y = game->y + 1; y < game->rows; y++) {
		for (int x = 0; x < (game->cols + 1) / 2; x++) {
			f(game->yCoords[y], game->xCoords[x], game->cells[y][x], false);
		}
		for (int x = (game->cols + 1) / 2; x < game->cols; x++) {
			f(game->yCoords[y], game->xCoords[x], game->cells[y][x], true);
		}
	}

	// finally deal with (game->y, game->x)
	attron(A_BOLD);
	f(game->yCoords[game->y], game->xCoords[game->x], 0, game->x >= (game->cols + 1) / 2);
	attroff(A_BOLD);
}

// calculate line coordinates and offset cell coordinates by
// amount corresponding to line coordinates
int initLines(const int dimPixels, const int dimCells, int lines[], int *coords) {
	const int margin = getMargin(dimPixels);
	getCoords(dimPixels - 2 * margin, dimCells, lines, coords);
	int offsets[dimCells];
	// the functions used return coordinates not relative to the edge of the screen
	// loop to add that in
	for (int i = 0; i < dimCells - 1; i++) {
		lines[i] += margin;
		coords[i] += margin;
	}
	coords[dimCells - 1] += margin; // one more line than cell
	return margin;
}

// draws the grid and draws initial set of numbers
// stores cell coordinates in yCoords and xCoords
void init(GameVars *game) {
	// ncurses initialization 
	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);	

	// skip (0, 0) because that will be drawn by coordinate
	for (int x = 1; x < game->cols; x++) {
		game->cells[0][x] = x;
	}
	for (int y = 1; y < game->rows; y++) {
		for (int x = 0; x < game->cols; x++) {
			game->cells[y][x] = y * game->cols + x;
		}
	}

	// init lines and cell coordinates
	int xLines[game->cols - 1];
	const int xMargin = initLines(COLS, game->cols, xLines, game->xCoords);
	int yLines[game->rows - 1];
	const int yMargin = initLines(LINES, game->rows, yLines, game->yCoords);

	// print gridlines
	for (int i = 0; i < game->cols - 1; i++) {
		mvvline(yMargin, xLines[i], '|', LINES - 2 * yMargin);
	}
	for (int i = 0; i < game->rows - 1; i++) {
		mvhline(yLines[i], xMargin, '-', COLS - 2 * xMargin);
	}
	for (int x = 0; x < game->cols - 1; x++) {
		for (int y = 0; y < game->rows - 1; y++) {
			mvaddch(yLines[y], xLines[x], '+');
		}
	}

	cellsMap(game, drawNum);
}
