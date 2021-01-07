#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "drawing.h"
#include "randomization.h"
/* #include "test.h" */

int main(int argc, char *argv[]) {
	int cols, rows;
	bool needToSeed = true;
	long int seed;

	opterr = 0;
	int c;
	while ((c = getopt(argc, argv, "s:")) != -1) {
		switch (c) {
			case 's':
				seed = atol(optarg);
				needToSeed = false;
				srand(seed);
				break;
			case ':':
				fprintf(stderr, "Seed option must take value\n");
				exit(1);
			case '?':
				fprintf(stderr, "Unknown option %c\n", optopt);
				exit(2);
		}
	}

	if (needToSeed) {
		seed = time(0);
		srand(seed);
	}
	argc -= optind;
	if (argc == 0) {
		cols = rows = 4;
	}
	else if (argc == 2) {
		rows = atoi(argv[optind]);
		cols = atoi(argv[optind + 1]);
		if (rows < 2 || cols < 2) {
			fprintf(stderr, "The game breaks when either dimension has a value less than 2\n");
			exit(3);
		}
	}
	else {
		printf("Usage: ./npuzzle [columns rows] [-s seed]\nseed must be a long int\n");
		exit(4);
	}
	
	// game init
	int cells[rows][cols];
	int yCoords[rows];
	int xCoords[cols];
	init(rows, cols, cells, yCoords, xCoords);

	int x, y;
	x = y = 0;

	// game loop
	while ((c = getch()) != 'q' && c != 'Q') {
		int swapx, swapy;
		
		switch (c) {
			case 's':
				mvprintw(0, 0, "%li", seed);
				continue;
			// randomize board
			case 'r':
				randomize(&y, &x, rows, cols, yCoords, xCoords, cells);
				continue;
			// ai solve
			case 'a':
			{
				continue;
			}
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
		swap0(&y, &x, swapy, swapx, cols, cells, xCoords, yCoords);
	}
	endwin();
}
