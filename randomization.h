#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "undo.h"
#include "drawing.h"
// #include "test.h"

#include "globals.h"

// randomizes ith element of cells with element of nums
// returns whether or not chosen num is 0
bool fillRand(int nums[], const int i, const int length) {
	const int choice = rand() % (length - i);
	cells[i / cols][i % cols] = nums[choice];
	nums[choice] = nums[length - i - 1];
	return choice == 0;
}

// randomize board
void randomize() {
	// clear board
	cellsMap(clearSpot); 

	// will randomize by:
	// 1. creating array of ints from 0 to length - 1
	// 2. for i in range(length):
	// 	1. pick random element and append to cells
	// 	2. replace that element with element at end of list
	// 	3. reduce array length by 1
	// we won't actually resize the array but will do so conceptually
	
	// create array of ints
	int nums[rows * cols];
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			nums[i * cols + j] = i * cols + j;
		}
	}

	// randomize
	const int length = rows * cols;
	int i = 0;
	for (; i < length; i++) {
		// need to keep track of the 0 to set its coords
		if (fillRand(nums, i, length)) {
			yCoord = i / cols;
			xCoord = i % cols;
			i++;
			break;
		}
	}
	for (; i < length; i++) {
		// already found the 0 so no need to check for it
		// fillRand(cells, nums, i / cols, i % cols, length - i);
		// const int choice = rand() % (length - i);
		// cells[i / cols][i % cols] = nums[choice];
		// nums[choice] = nums[length - i - 1];
		fillRand(nums, i, length);
	}

	// make sure the board is actually solvable
	//
	// credit for this algorithm goes to Chris Calabro
	// http://cseweb.ucsd.edu/~ccalabro/essays/15_puzzle.pdf
	// gives sign of inversion in O(length) iterations
	//
	// copy cells to an array its ok to modify
	int copy[length];
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			copy[i * cols + j] = cells[i][j];
		}
	}

	bool parity = false;
	i = 0;
	while (i < length) {
		if (i != copy[i]) {
			// swap copy[i] and copy[copy[i]]
			const int temp = copy[i];
			copy[i] = copy[temp];
			copy[temp] = temp;

			parity = !parity; 
		}   
		else {
			i++;
		}   
	}
	parity = parity != ((xCoord + yCoord) % 2); // xor parity with the parity of manhattan distance of 0 to its goal position

	// board is not solvable if odd parity
	// if so, swap 2 arbitrary non 0 cells
	if (parity) {
		cells[yCoord][xCoord] = cells[(yCoord + 1) % rows][xCoord]; // use 0 cell as temp
		cells[(yCoord + 1) % rows][xCoord] = cells[(yCoord + 1) % rows][(xCoord + 1) % cols];
		cells[(yCoord + 1) % rows][(xCoord + 1) % cols] = cells[yCoord][xCoord];
	}

	cellsMap(drawNum); // redraw all numbers
}
