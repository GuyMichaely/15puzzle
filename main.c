#include <ncurses.h>
#include <stdio.h>

int main() {
	initscr();
	
	// ncurses initialization 
	noecho();
	raw();
	keypad(stdscr, TRUE);

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

	getch();
	// refresh
	endwin();
}

