#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "drawing.h"
#include "randomization.h"
#include "ai.h"
#include "undo.h"
/* #include "test.h" */

int cols, rows, yCoord, xCoord;
int *yCoords, *xCoords;
int **cells;

// undo the move represented by **move
void undoMove(Move **move) {
	if (*move != NULL) {
		switch ((*move)->move) {
			case 'u':
				swap0NoUndo(yCoord - 1, xCoord);
				break;
			case 'l':
				swap0NoUndo(yCoord, xCoord - 1);
				break;
			case 'd':
				swap0NoUndo(yCoord + 1, xCoord);
				break;
			case 'r':
				swap0NoUndo(yCoord, xCoord + 1);
				break;
		}
		const Move *toFree = *move;
		*move = (*move)->prev;
		free(toFree);
	}
}

int main(int argc, char *argv[]) {
	bool needToSeed = true;
	long int seed;

	// cmd line parsing
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
	cells = malloc(rows * sizeof(int *));
	for (int i = 0; i < rows; i++) {
		cells[i] = malloc(cols * sizeof(int));
	}
	yCoords = malloc(rows * sizeof(int));
	xCoords = malloc(cols * sizeof(int));
	init();

	xCoord = yCoord = 0;

	Move* undo = NULL;

	// game loop
	while ((c = getch()) != 'q' && c != 'Q') {
		int swapx, swapy;
		
		switch (c) {
			case 's':
				mvprintw(0, 0, "%li", seed);
				continue;
			// undo move
			case 'u':
				undoMove(&undo);
				continue;
			// randomize board
			case 'r':
				randomize();
			case 'c':
				freeMoves(undo);
				undo = NULL;
				continue;
			// ai solve
			case 'a':
			{
				ai(&undo);
				continue;
			}
			// movement controls
			case KEY_UP:
			case 'k':
				if (yCoord != 0) {
					swap0(yCoord - 1, xCoord, &undo, 'd');
				}
				continue;
			case KEY_DOWN:
			case 'j':
				if (yCoord != rows - 1) {
					swap0(yCoord + 1, xCoord, &undo, 'u');
				}
				continue;
			case KEY_LEFT:
			case 'h':
				if (xCoord != 0) {
					swap0(yCoord, xCoord - 1, &undo, 'r');
				}
				continue;
			case KEY_RIGHT:
			case 'l':
				if (xCoord != cols - 1) {
					swap0(yCoord, xCoord + 1, &undo, 'l');
				}
				continue;
		}
	}

	freeMoves(undo);
	endwin();
}
