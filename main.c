#include <ncurses.h>
#include <stdio.h>

int main() {
	int row, col;
	char s[100];
	printf("row, col, and string to print: "); 
	scanf("%i %i %s", &row, &col, s);

	initscr();
	
	// ncurses initialization 
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
	mvprintw(row, col, s);
	refresh();
	getch();
	
	endwin();
}

