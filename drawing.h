#pragma once

#include <ncurses.h>
#include <stdlib.h>
// #include "test.h"

#include "undo.h"

#include "globals.h"

inline static void midPrint(int row, char *string) {
	mvprintw(row, (COLS - strlen((string))) / 2, string);
}

// d is the length of the dimension
// margin length and grid length will be stored in *margin, *length
int getMargin(int d){
	const int modLimit = 1; // if d % 6 > 1 margin has enough room to be 1 bigger
	return d / 6 + (d % 6 > modLimit);
}

// given the length of the dimension and number of cells to split it into
// stores line coordinates (relative to edge of box, not edge of screen)_in lineCoords
// returns length of last cell
int getCoords(int length, int split, int lineCoords[], int cellCoords[]) {
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
	
	return length - lineCoords[split - 2] - 1; // - 1 because the thickness of the line takes up space
}

// only for nonnegative integers
int intLength(int x) {
	int length = 1;
	for (int lt = 9; lt < x; lt = lt * 10 + 9) {
		length++;
	}
	return length;
}

// when drawing numbers in grid, different numbers will need to be offset differently
// stores offsets in offsets
void getOffsets(const int lines, const int lastLength, const int lineCoords[], int offsets[]) {
	offsets[lines / 2] = lastLength % 2;
	for (int i = lines / 2 + 1; i < lines; i++) {
		offsets[0] = !((lineCoords[i] + lineCoords[ i - 1]) % 2);
	}
}

// draws number at position, properly centering it
void drawNum(const int y, const int x, const int num, const bool onRight) {
	mvprintw(y, x - (intLength(num) - onRight) / 2, "%i", num);
}

// clears out cell at coordinate yCoord, xCoord
void clearSpot(const int yCoord, int xCoord, const int value, const bool onRight) {
	const int length = intLength(value);
	xCoord -= (length - onRight) / 2;
	mvhline(yCoord, xCoord, ' ', length);
}

// swap 0 cell with (swapy, swapx)
// do not add move to the undo list
void swap0NoUndo(const int swapy, const int swapx) {
	const bool xOnRight     =     xCoord >= (cols + 1) / 2;
	const bool swapXOnRight = swapx >= (cols + 1) / 2;

	clearSpot(yCoords[yCoord], xCoords[xCoord], 0, xOnRight);
	drawNum(yCoords[yCoord], xCoords[xCoord], cells[swapy][swapx], xOnRight);

	clearSpot(yCoords[swapy], xCoords[swapx], cells[swapy][swapx], swapXOnRight);
	attron(A_BOLD);
	drawNum(yCoords[swapy], xCoords[swapx], 0, swapXOnRight);
	attroff(A_BOLD);
	cells[yCoord][xCoord] = cells[swapy][swapx];
	
	xCoord = swapx;
	yCoord = swapy;
}

// swap 0 cell with (swapy, swapx)
// add the swap to the undo list
void swap0(const int swapy, const int swapx, Move **undo, char undoDirection) {
	swap0NoUndo(swapy, swapx);


	// add move to undo list
	Move* newMove = malloc(sizeof(Move));
	newMove->move = undoDirection;
	newMove->prev = *undo;
	*undo = newMove;

	refresh();
}

// executes f on every cell
// skips (yCoord, xCoord) and executes f on that cell last
void cellsMap(void (*f)(int, int, int, bool)) {
	// iterate up to yCoord row
	for (int y = 0; y < yCoord; y++) {
		for (int x = 0; x < (cols + 1) / 2; x++) {
			f(yCoords[y], xCoords[x], cells[y][x], false);
		}
		for (int x = (cols + 1) / 2; x < cols; x++) {
			f(yCoords[y], xCoords[x], cells[y][x], true);
		}
	}

	// iterate up to either halfway or xCoord
	int x = 0;
	for (; x < xCoord && x < (cols + 1) / 2; x++) {
		f(yCoords[yCoord], xCoords[x], cells[yCoord][x], false);
	}

	// if stopped because reached xCoord, iterate until halfway
	// else if stopped because reached halfway, continue until reach xCoord
	// then continue
	if (x == xCoord) {
		x++;
		for (; x < (cols + 1) / 2; x++) {
			f(yCoords[yCoord], xCoords[x], cells[yCoord][x], false);
		}
	}
	else {
		for (; x < xCoord; x++) {
			f(yCoords[yCoord], xCoords[x], cells[yCoord][x], true);
		}
		x++;
	}
	for (; x < cols; x++) {
		f(yCoords[yCoord], xCoords[x], cells[yCoord][x], true);
	}

	// iterate through the rest
	for (int y = yCoord + 1; y < rows; y++) {
		for (int x = 0; x < (cols + 1) / 2; x++) {
			f(yCoords[y], xCoords[x], cells[y][x], false);
		}
		for (int x = (cols + 1) / 2; x < cols; x++) {
			f(yCoords[y], xCoords[x], cells[y][x], true);
		}
	}

	// finally deal with (yCoord, xCoord)
	attron(A_BOLD);
	f(yCoords[yCoord], xCoords[xCoord], 0, xCoord >= (cols + 1) / 2);
	attroff(A_BOLD);
}

// draws the grid and draws initial set of numbers
// stores cell coordinates in yCoords and xCoords
void init() {
	// ncurses initialization 
	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);	

	// skip (0, 0) because that will be drawn by coordinate
	for (int x = 1; x < cols; x++) {
		cells[0][x] = x;
	}
	for (int y = 1; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			cells[y][x] = y * cols + x;
		}
	}

	// get xlines and xcells
	int xLines[cols - 1];
	int xMargin = getMargin(COLS);
	const int lastLength = getCoords(COLS - 2 * xMargin, cols, xLines, xCoords);
	int offsets[cols];
	getOffsets(cols - 1, lastLength, xCoords, offsets);
	// the functions used return coordinates not relative to the edge of the screen
	// loop to add that in
	for (int i = 0; i < cols - 1; i++) {
		xLines[i] += xMargin;
		xCoords[i] += xMargin;
	}
	xCoords[cols - 1] += xMargin;

	// get ylines and ycells
	int yMargin = getMargin(LINES);
	int yLines[rows - 1];
	getCoords(LINES - 2 * yMargin, rows, yLines, yCoords);

	for (int i = 0; i < rows - 1; i++) {
		yLines[i] += yMargin;
		yCoords[i] += yMargin;
	}
	yCoords[rows - 1] += yMargin;

	// print gridlines
	for (int i = 0; i < cols - 1; i++) {
		mvvline(yMargin, xLines[i], '|', LINES - 2 * yMargin);
	}
	for (int i = 0; i < rows - 1; i++) {
		mvhline(yLines[i], xMargin, '-', COLS - 2 * xMargin);
	}
	for (int x = 0; x < cols - 1; x++) {
		for (int y = 0; y < rows - 1; y++) {
			mvaddch(yLines[y], xLines[x], '+');
		}
	}

	cellsMap(drawNum);
}
