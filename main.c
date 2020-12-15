#include <ncurses.h>
#include <stdio.h>
#include <unistd.h>

// also includes '+' for intersections with vertical seperators
void horizontalSeperators(const int y, const int l) {
	mvhline(y, COLS / 3, '-', l);
	mvaddch(y, COLS / 3 + l, '+');
	mvhline(y, COLS / 3 + l + 1, '-', l);
	mvaddch(y, COLS / 3 + 2 * l + 1, '+');
	mvhline(y, COLS / 3 + 2 * l + 2, '-', l);
}

void verticalSeperators(int x, const int l) {
	mvvline(LINES / 3, x, '|', l);
	mvvline(LINES / 3 + l + 1, x, '|', l);
	mvvline(LINES / 3 + 2 * l + 2, x, '|', l);
}

void init() {
	// grid will take up 1/9 of screen
	// and will be centered
	
	const int HOZ  = COLS / 9.0 - 2.0 / 3;  // 3 * length + 2 = DIM / 3 
	horizontalSeperators(LINES * 4 / 9, HOZ);
	horizontalSeperators(LINES * 5 / 9, HOZ);

	const int VERT = LINES / 9.0 - 2.0 / 3;  // 3 * length + 2 = DIM / 3 
	verticalSeperators(COLS * 4 / 9, VERT);
	verticalSeperators(COLS * 5 / 9, VERT);

	//mvhline(LINES / 2, COLS / 2 - 10, '-', 20);
	refresh();
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


