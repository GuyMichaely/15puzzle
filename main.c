#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "game_vars.h"
#include "drawing.h"
#include "randomization.h"
#include "ai.h"
#include "undo.h"
/* #include "test.h" */

// undo the move represented by **move
void undoMove(GameVars *game, Move **move) {
	if (*move != NULL) {
		switch ((*move)->move) {
			case 'u':
				swap0NoUndo(game, -1, 0);
				break;
			case 'l':
				swap0NoUndo(game, 0, -1);
				break;
			case 'd':
				swap0NoUndo(game, 1, 0);
				break;
			case 'r':
				swap0NoUndo(game, 0, 1);
				break;
		}
		const Move *toFree = *move;
		*move = (*move)->prev;
		free(toFree);
	}
}

int main(int argc, char *argv[]) {
	GameVars game;
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
		game.cols = game.rows = 4;
	}
	else if (argc == 2) {
		game.rows = atoi(argv[optind]);
		game.cols = atoi(argv[optind + 1]);
		if (game.rows < 2 || game.cols < 2) {
			fprintf(stderr, "The game breaks when either dimension has a value less than 2\n");
			exit(3);
		}
	}
	else {
		printf("Usage: ./npuzzle [columns rows] [-s seed]\nseed must be a long int\n");
		exit(4);
	}
	
	// game init
	game.cells = malloc(game.rows * sizeof(int *));
	for (int i = 0; i < game.rows; i++) {
		game.cells[i] = malloc(game.cols * sizeof(int));
	}
	game.yCoords = malloc(game.rows * sizeof(int));
	game.xCoords = malloc(game.cols * sizeof(int));
	game.y = game.x = 0;
	init(&game);

	game.x = game.y = 0;

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
				undoMove(&game, &undo);
				continue;
			// randomize board
			case 'r':
				randomize(&game);
			case 'c':
				freeMoves(undo);
				undo = NULL;
				continue;
			// ai solve
			case 'a':
			{
				ai(&game, &undo);
				continue;
			}
			// movement controls
			case KEY_UP:
			case 'k':
				if (game.y != 0) {
					swap0(&game, -1, 0, &undo, 'd');
				}
				continue;
			case KEY_DOWN:
			case 'j':
				if (game.y != game.rows - 1) {
					swap0(&game, 1, 0, &undo, 'u');
				}
				continue;
			case KEY_LEFT:
			case 'h':
				if (game.x != 0) {
					swap0(&game, 0, -1, &undo, 'r');
				}
				continue;
			case KEY_RIGHT:
			case 'l':
				if (game.x != game.cols - 1) {
					swap0(&game, 0, 1, &undo, 'l');
				}
				continue;
		}
	}

	freeMoves(undo);
	endwin();
}
