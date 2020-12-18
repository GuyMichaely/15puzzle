#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// d is the length of the dimension
// margin length and grid length will be stored in *margin, *length
int getMargin(int d){
	const int modLimit = 1; // if d % 6 > 1 margin has enough room to be 1 bigger
	return d / 6 + (d % 6 > modLimit);
}

void interactiveDebug(char *w, int v) {
	mvprintw(0, 0, "                   ");
	mvprintw(0, 0, w, v);
	if (getch() == 'q') {
		endwin();
		exit(0);
	}
}

// given the length of the dimension and number of cells to split it into
// stores line coordinates (relative to edge of box, not edge of screen)_in lineCoords
void getLineCoords(int length, int cells, int lineCoords[]) {
	// most likely won't be able to divide the grid perfectly evenly
	// therefore spread the larger ones out throughtout the dimension
	// symmetrically and evenly

	// if there is an odd number of lines set the middle one
	if (!(cells % 2)) {
		lineCoords[cells / 2 - 1] = length / 2;
	}

	// iterate through gridlines out to in
	// set each gridline symmetrically on the left and right of the center
	for (int i = 0; i < (cells + 1) / 2 - 1; i++) {
		lineCoords[i] = length * (i + 1) / cells;
		lineCoords[(cells - 2) - i] = length - lineCoords[i]; // (cells - 2) bc number of lines is cells - 1
		// then subtract 1 again to avoid overflow; cells - 2
	}
}

// given coordinates of cell seperators
// stores where the numbers should be placed along the dimension in cellCoords
void getCellCoords(int firstLength, const int lineCoords[], int cellCoords[]) {

}

void init(int rows, int cols) {
	int xMargin = getMargin(COLS);
	int xLines[cols - 1];
	getLineCoords(COLS - 2 * xMargin, cols, xLines);

	int yMargin = getMargin(LINES);
	int yLines[rows - 1];
	getLineCoords(LINES - 2 * yMargin, rows, yLines);

	for (int i = 0; i < cols - 1; i++) {
		mvvline(yMargin, xLines[i] + xMargin, '|', LINES - 2 * yMargin);
	}
	for (int i = 0; i < rows - 1; i++) {
		mvhline(yLines[i] + yMargin, xMargin, '-', COLS - 2 * xMargin);
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


