#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define interactiveDebug(...) { \
	mvprintw(0, 0, "                                                                                                                                                                            "); \
	mvprintw(0, 0, __VA_ARGS__); \
	if (getch() == 'q') { \
		endwin(); \
		exit(0); \
	} \
}
	
// d is the length of the dimension
// margin length and grid length will be stored in *margin, *length
int getMargin(int d){
	const int modLimit = 1; // if d % 6 > 1 margin has enough room to be 1 bigger
	return d / 6 + (d % 6 > modLimit);
}

// given the length of the dimension and number of cells to split it into
// stores line coordinates (relative to edge of box, not edge of screen)_in lineCoords
int getCoords(int length, int cells, int lineCoords[], int cellCoords[]) {
	// most likely won't be able to divide the grid perfectly evenly
	// therefore spread the larger ones out throughtout the dimension
	// symmetrically and evenly

	// want to calculate seperators as though the lines don't take up space
	// will account for the difference when storing the data
	length -= (cells - 1);

	// if there is an odd number of lines set the middle one
	if (!(cells % 2)) {
		lineCoords[cells / 2 - 1] = length / 2 + (cells - 1) / 2; // (+ (cells - 1) / 2 to account for the space the lines take
	}

	// iterate through gridlines out to in
	// set each gridline symmetrically on the left and right of the center
	const int lastIndex = (cells + 1) / 2 - 1;
	for (int i = 0; i < lastIndex; i++) {
		lineCoords[i] = length * (i + 1) / cells + i;
		lineCoords[(cells - 2) - i] = length - lineCoords[i] + ((cells - 1) - 1); // + (cells - 1) to get length back to where it was, - 1 because last line is always too short
	}

	// if there's an odd number of lines
	// it can happen that the (n / 2 + 1)th section is too large
	// check and account for that
	if (!(cells % 2)) {
		const int middle = cells / 2 - 1;
		if (lineCoords[middle + 2] - lineCoords[middle + 1] > 1) {
			lineCoords[middle + 1]--;
		}
	}

	// calculate cell coordinates
	length += (cells - 1);

	cellCoords[0] = (lineCoords[0]) / 2;
	cellCoords[cells - 1] = (length + lineCoords[cells - 2]) / 2;
	for (int i = 1; i < (cells + 1) / 2; i++) {
		cellCoords[cells - 1 - i] = (lineCoords[cells - 1 - i] + lineCoords[cells - 2 - i]) / 2;
		cellCoords[i] = (lineCoords[i] + lineCoords[i - 1] + 1) / 2;
	}
	
	/*
	interactiveDebug("length: %i", length);
	for (int i = 0; i < cells - 1; i++) {
		interactiveDebug("lineCoords[%i]: %i", i, lineCoords[i]);
	}
	for (int i = 0; i < cells; i++) {
		interactiveDebug("cellCoords[%i]: %i", i, cellCoords[i]);
	}
	*/
	return length - lineCoords[cells - 2] - 1; // - 1 because the thickness of the line takes up space
}

// only for positive integers
int intLength(int x) {
	int length = 1;
	for (int lt = 10; lt - 1 < x; lt = lt * 10) {
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

void init(int rows, int cols) {
	int xLines[cols - 1];
	int xCells[cols];
	int xMargin = getMargin(COLS);
	const int lastLength = getCoords(COLS - 2 * xMargin, cols, xLines, xCells);
	int offsets[cols];
	getOffsets(cols - 1, lastLength, xCells, offsets);

	int yMargin = getMargin(LINES);
	int yLines[rows - 1];
	int yCells[rows];
	getCoords(LINES - 2 * yMargin, rows, yLines, yCells);

	// print gridlines
	for (int i = 0; i < cols - 1; i++) {
		mvvline(yMargin, xLines[i] + xMargin, '|', LINES - 2 * yMargin);
	}
	for (int i = 0; i < rows - 1; i++) {
		mvhline(yLines[i] + yMargin, xMargin, '-', COLS - 2 * xMargin);
	}
	for (int x = 0; x < cols - 1; x++) {
		for (int y = 0; y < rows - 1; y++) {
			mvaddch(yLines[y] + yMargin, xLines[x] + xMargin, '+');
		}
	}

	// draw numbers in cells
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < (cols + 1) / 2; x++) {
			const int yCoord = yCells[y] + yMargin;
			int xCoord = xCells[x] + xMargin;
			xCoord -= intLength(cols * y + x) / 2;
			mvprintw(yCoord, xCoord, "%i", cols * y + x);
		}
		for (int x = (cols + 1) / 2; x < cols; x++) {
			const int yCoord = yCells[y] + yMargin;
			int xCoord = xCells[x] + xMargin;
			xCoord -= (intLength(cols * y + x) - 1) / 2;
			mvprintw(yCoord, xCoord, "%i", cols * y + x);
		}
	}
}

int main(int argc, char *argv[]) {
	int cols, rows;
	if (argc == 1) {
		cols = rows = 2;
	}
	else if (argc != 3) {
		printf("Usage: npuzzle [rows columns]\n");
		return 1;
	}
	else {	
		rows = atoi(argv[1]);
		cols = atoi(argv[2]);
	}
	if (rows < 2 || cols < 2) {
		printf("Both rows and columns must be more than 1\n");
		return 2;
	}


	// ncurses initialization 
	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);
	/*
	// key detection and printw
	int c = getch();
	switch (c) {
		case KEY_UP:
		case 'k':
			printw("Up");
			break;
		case KEY_DOWN:
		case 'j':
			printw("Down");
			break;
	}
	*/
	
	/*
	initial draw;
	getch;
	while (getch is not exit) {
		
	}
	exit; */
	init(rows, cols);
	char c;
	int i;
	while ((c = getch()) != 'q') {
		mvaddch(0, i++, c);
		usleep(100000);
	}
	endwin();
}


