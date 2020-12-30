#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "griddrawing.h"
#include "inversions.h"
#include "test.h"

// fills in next spot in cells with random available number in nums
#define fillRand(cells, nums, y, x, listSize) \
do { \
	int choice = rand() % (listSize); \
	cells[(y)][(x)] = nums[choice]; \
	nums[choice] = nums[(listSize) - 1]; \
} while (false)

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
		
		switch (c) {
			// randomize board
			case 'r':
			// add blocks so variable decleration works as you would want
			// i don't really know how it works ive just had bad experiences with switch in the past
			{
				// clear board
				cellsMap(rows, cols, yCoords, xCoords, cells, clearSpot);

				// create array of numbers to be put in grid
				int nums[rows * cols];
				for (int y = 0; y < rows; y++) {
					for (int x = 0; x < cols; x++) {
						nums[y * cols + x] = y * cols + x;
					}
				}

				// randomize board
				srand(time(0));
				const int length = rows * cols;
				int i = 0;
				for (; i < length; i++) {
					y = i / rows;
					x = i % cols;
					fillRand(cells, nums, y, x, length - i);
					// need to keep track of the 0 to set its coords
					if (cells[y][x] == 0) {
						i++;
						break;
					}
				}
				for (; i < length; i++) {
					// already found the 0 so no need to check for it
					fillRand(cells, nums, i / rows, i % cols, length - i);
				}
				
				// make sure the board is actually solvable
				// a board is solvable (able to get back to the original state)
				// iff the parity of the grid permutation's inversion number
				// + the manhattan distance of the 0 cell to the top left is even.
				// if our randomization isn't, will swap 2 cells to make it so
				// 
				// calculate inversion parity with merge sort
				// first copy the permutation in to nums
				for (int y = 0; y < rows; y++) {
					for (int x = 0; x < cols; x++) {
						nums[y * cols + x] = cells[y][x];
					}
				}
				bool parity = mergeSortInversions(nums, nums + length);// calculate inversion parity
				parity ^= (x + y) % 2; // parity of manhattan distance of movable cell
				
				// board is not solvable if odd parity
				// if so, swap 2 arbitrary non 0 cells
				if (parity) {
					const int temp = cells[(y + 1) % rows][x];
					cells[(y + 1) % rows][x] = cells[(y + 1) % rows][(x + 1) % cols];
					cells[(y + 1) % rows][(x + 1) % cols] = temp;
				}
				
				cellsMap(rows, cols, yCoords, xCoords, cells, drawNum); // redraw all numbers
			}
				continue;
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
