#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "griddrawing.h"

#define interactiveDebug(...) { \
	mvprintw(0, 0, "                                                                                                                                                                            "); \
	mvprintw(0, 0, __VA_ARGS__); \
	if (getch() == 'q') { \
		endwin(); \
		exit(0); \
	} \
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

	// game init
	int cells[rows][cols];
	int yCoords[rows];
	int xCoords[cols];
	init(rows, cols, cells, yCoords, xCoords);

	int c;
	int x, y;
	x = y = 0;

	// game loop
	while ((c = getch()) != 'q' && c != 'Q') {
		int swapx, swapy;
		int i = 0;
		
		switch (c) {
			// movement controls
			case KEY_UP:
			case 'k':
				if (y != 0) {
					swapy = y - 1;
					swapx = x;
				}
				else {
					continue;
				}
				break;
			case KEY_DOWN:
			case 'j':
				if (y != rows - 1) {
					swapy = y + 1;
					swapx = x;
				}
				else {
					continue;
				}
				break;
			case KEY_LEFT:
			case 'h':
				if (x != 0) {
					swapx = x - 1;
					swapy = y;
				}
				else {
					continue;
				}
				break;
			case KEY_RIGHT:
			case 'l':
				if (x != cols - 1) {
					swapx = x + 1;
					swapy = y;
				}
				else {
					continue;
				}
				break;
			default:
				continue;
		}
		swap(y, x, swapy, swapx, cols, cells, yCoords, xCoords);
		x = swapx;
		y = swapy;
		refresh();
	}
	endwin();
}
