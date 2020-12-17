#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>

// d is the length of the dimension
// margin length and grid length will be stored in *margin, *length
void getBounds(int d, int * const margin, int * const length) {
	// first figure out bounds of box
	*margin = d / 6;
	const int modLimit = 2;
	if (d % 6 < modLimit) { // if mod greater than modLimit I want to make the margins bigger
		*margin = d / 6;
	}
	else {
		*margin = d / 6 + 1;
	}

	// now figure out grid sizes
	d -= 2 * *margin + 2; // inner length = total length - 2 * *margin - 2 (-2 for the gridline widths)
	if (d % 3 == 2) { // mod 2 means extra space symmetryically on either side of center
		*length = d / 3 + 1;
	}
	else {
		*length = d / 3;
	}	
}

void init() {
	// x gridlines
	int xMargin, xGridWidth;
	getBounds(COLS, &xMargin, &xGridWidth);
	const int x1 = xMargin + xGridWidth + 1;
	const int x2 = COLS - xMargin - xGridWidth;

	// y gridlines
	int yMargin, yGridWidth;
	getBounds(LINES, &yMargin, &yGridWidth);
	const int y1 = yMargin + yGridWidth + 1;
	const int y2 = LINES - yMargin - yGridWidth;

	// draw the gridlines
	mvhline(y1, xMargin, '-', COLS - 2 * xMargin);
	mvhline(y2, xMargin, '-', COLS - 2 * xMargin);
	mvvline(yMargin, x1, '|', LINES - 2 * yMargin);
	mvvline(yMargin, x2, '|', LINES - 2 * yMargin);

	// intersections
	mvaddch(y1, x1, '+');
	mvaddch(y1, x2, '+');
	mvaddch(y2, x1, '+');
	mvaddch(y2, x2, '+');
}

	

int main() {
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
	init();
	char c;
	int i;
	while ((c = getch()) != 'q') {
		mvaddch(0, i++, c);
		usleep(100000);
	}
	endwin();
}


