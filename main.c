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
void getCoords(int length, int cells, int lineCoords[], int cellCoords[]) {
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

	// calculate cell coordinates
	length += (cells - 1);
	cellCoords[0] = (lineCoords[0] - 1) / 2;
	cellCoords[cells - 1] = length - cellCoords[0] - 1; // - 1 because 1 too far to the right otherwise
	for (int i = 1; i < (cells + 1) / 2; i++) {
		cellCoords[i] = (lineCoords[i] + lineCoords[i - 1]) / 2;
		cellCoords[cells - 1 - i] = length - cellCoords[i] - 1;
	}
}

void init(int rows, int cols) {
	int xMargin = getMargin(COLS);
	int xLines[cols - 1];
	int xCells[cols];
	getCoords(COLS - 2 * xMargin, cols, xLines, xCells);

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
	// demo print cells
	for (int x = 0; x < cols; x++) {
		for (int y = 0; y < rows; y++) {
			mvprintw(yCells[y] + yMargin, xCells[x] + xMargin, "x");
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


