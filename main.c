#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>

#include "game_vars.h"
#include "drawing.h"
#include "randomization.h"
#include "ai.h"
#include "undo.h"
/* #include "test.h" */

// undo the move represented by **move
void undoMove(GameVars *game) {
	if (game->undo != NULL) {
		// perform most recent undo
		switch (game->undo->move) {
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

		// delete most recent undo
		const Move *toFree = game->undo;
		game->undo = game->undo->prev;
		free(toFree);
	}
}

long int seed; // remove after done with debug and replace with decleration in main
void printSeed() {
	printf("seed: %lii\n", seed);
}
int main(int argc, char *argv[]) {
	atexit(printSeed);
	bool needToSeed = true;
	//long int seed; // uncomment after debug

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
	GameVars game;
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
	game.undo = NULL;
	init(&game);

	// game loop
	while ((c = getch()) != 'q' && c != 'Q') {
		int swapx, swapy;
		
		switch (c) {
			case 's':
				mvprintw(0, 0, "%li", seed);
				continue;
			// undo move
			case 'u':
				undoMove(&game);
				continue;
			// randomize board
			case 'r':
				randomize(&game);
			case 'c':
				freeMoves(game.undo);
				game.undo = NULL;
				continue;
			// ai solve
			case 'a':
			{
				ai(&game);
				continue;
			}
			// movement controls
			case KEY_UP:
			case 'k':
				if (game.y != 0) {
					swap0(&game, -1, 0, 'd');
				}
				continue;
			case KEY_DOWN:
			case 'j':
				if (game.y != game.rows - 1) {
					swap0(&game, 1, 0, 'u');
				}
				continue;
			case KEY_LEFT:
			case 'h':
				if (game.x != 0) {
					swap0(&game, 0, -1, 'r');
				}
				continue;
			case KEY_RIGHT:
			case 'l':
				if (game.x != game.cols - 1) {
					swap0(&game, 0, 1, 'l');
				}
				continue;
		}
	}

	freeMoves(game.undo);
	endwin();
}
