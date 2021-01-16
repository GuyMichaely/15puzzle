#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "undo.h"
#include "drawing.h"
#include "swap_ints.h"
#include "game_vars.h"

// randomizes ith element of game->cells with element of nums
// returns whether or not chosen num is 0
bool fillRand(GameVars *game, int nums[], const int i, const int length) {
	const int choice = rand() % (length - i);
	game->cells[i / game->cols][i % game->cols] = nums[choice];
	nums[choice] = nums[length - i - 1];
	return choice == 0;
}

// randomize board
void randomize(GameVars* game) {
	// clear board
	cellsMap(game, clearSpot); 

	// will randomize by:
	// 1. creating array of ints from 0 to length - 1
	// 2. for i in range(length):
	// 	1. pick random element and append to cells
	// 	2. replace that element with element at end of list
	// 	3. reduce array length by 1
	// we won't actually resize the array but will do so conceptually
	
	// create array of ints
	int nums[game->rows * game->cols];
	for (int i = 0; i < game->rows; i++) {
		for (int j = 0; j < game->cols; j++) {
			nums[i * game->cols + j] = i * game->cols + j;
		}
	}

	// randomize
	const int length = game->rows * game->cols;
	int i = 0;
	for (; i < length; i++) {
		// need to keep track of the 0 to set its coords
		if (fillRand(game, nums, i, length)) {
			game->y = i / game->cols;
			game->x = i % game->cols;
			i++;
			break;
		}
	}
	for (; i < length; i++) {
		fillRand(game, nums, i, length); // already found the 0 so no need to check for it
	}

	// make sure the board is actually solvable
	//
	// credit for this algorithm goes to Chris Calabro
	// http://cseweb.ucsd.edu/~ccalabro/essays/15_puzzle.pdf
	// gives sign of inversion in O(length) iterations
	//
	// copy game->cells to an array its ok to modify
	int copy[length];
	for (int i = 0; i < game->rows; i++) {
		for (int j = 0; j < game->cols; j++) {
			copy[i * game->cols + j] = game->cells[i][j];
		}
	}

	bool parity = false;
	i = 0;
	while (i < length) {
		if (i != copy[i]) {
			swapInts(copy + i, copy + copy[i]);
			parity = !parity; 
		}   
		else {
			i++;
		}   
	}
	parity = parity != ((game->x + game->y) % 2); // xor parity with the parity of manhattan distance of 0 to its goal position

	// board is not solvable if odd parity
	// if so, swap 2 arbitrary non 0 cells
	if (parity) {
		game->cells[game->y][game->x] = game->cells[(game->y + 1) % game->rows][game->x]; // use 0 cell as temp
		game->cells[(game->y + 1) % game->rows][game->x] = game->cells[(game->y + 1) % game->rows][(game->x + 1) % game->cols];
		game->cells[(game->y + 1) % game->rows][(game->x + 1) % game->cols] = game->cells[game->y][game->x];
	}

	cellsMap(game, drawNum); // redraw all numbers
}
