#include <ncurses.h>

// d is the length of the dimension
// margin length and grid length will be stored in *margin, *length
int getMargin(int d){
	const int modLimit = 1; // if d % 6 > 1 margin has enough room to be 1 bigger
	return d / 6 + (d % 6 > modLimit);
}

// given the length of the dimension and number of cells to split it into
// stores line coordinates (relative to edge of box, not edge of screen)_in lineCoords
// returns length of last cell
int getCoords(int length, int cells, int lineCoords[], int cellCoords[]) {
	// most likely won't be able to divide the grid perfectly evenly
	// therefore spread the larger ones out throughtout the dimension
	// symmetrically and evenly

	// want to calculate seperators as though the lines don't take up space
	// will account for the difference when storing the data
	length -= (cells - 1);

	// if there is an odd number of lines set the middle one
	if (!(cells % 2)) {
		lineCoords[cells / 2 - 1] = length / 2 + (cells - 1) / 2; // (+ (cells - 1) / 2 to account for the space the lines take
	}

	// iterate through gridlines out to in
	// set each gridline symmetrically on the left and right of the center
	const int lastIndex = (cells + 1) / 2 - 1;
	for (int i = 0; i < lastIndex; i++) {
		lineCoords[i] = length * (i + 1) / cells + i;
		lineCoords[(cells - 2) - i] = length - lineCoords[i] + ((cells - 1) - 1); // + (cells - 1) to get length back to where it was, - 1 because last line is always too short
	}

	// if there's an odd number of lines
	// it can happen that the (n / 2 + 1)th section is too large
	// check and account for that
	if (cells != 2 && !(cells % 2)) {
		const int middle = cells / 2 - 1;
		if ((lineCoords[middle + 1] - lineCoords[middle]) - (lineCoords[middle + 2] - lineCoords[middle + 1]) > 1) {
			lineCoords[middle + 1]--;
		}
	}

	// calculate cell coordinates
	length += (cells - 1);

	cellCoords[0] = (lineCoords[0]) / 2;
	cellCoords[cells - 1] = (length + lineCoords[cells - 2]) / 2;
	for (int i = 1; i < (cells + 1) / 2; i++) {
		cellCoords[cells - 1 - i] = (lineCoords[cells - 1 - i] + lineCoords[cells - 2 - i]) / 2;
		cellCoords[i] = (lineCoords[i] + lineCoords[i - 1] + 1) / 2;
	}
	
	return length - lineCoords[cells - 2] - 1; // - 1 because the thickness of the line takes up space
}

// only for nonnegative integers
int intLength(int x) {
	int length = 1;
	for (int lt = 9; lt < x; lt = lt * 10 + 9) {
		length++;
	}
	return length;
}

// when drawing numbers in grid, different numbers will need to be offset differently
// stores offsets in offsets
void getOffsets(const int lines, const int lastLength, const int lineCoords[], int offsets[]) {
	offsets[lines / 2] = lastLength % 2;
	for (int i = lines / 2 + 1; i < lines; i++) {
		offsets[0] = !((lineCoords[i] + lineCoords[ i - 1]) % 2);
	}
}

// draws number at position, properly centering it
void drawNum(const int y, const int x, const int num, const bool onRight) {
	mvprintw(y, x - (intLength(num) - onRight) / 2, "%i", num);
}

// clears out cell at coordinate yCoord, xCoord
void clearSpot(const int yCoord, int xCoord, const int value, const bool onRight) {
	const int length = intLength(value);
	xCoord -= (length - onRight) / 2;
	mvhline(yCoord, xCoord, ' ', length);
}

// executes f on every cell
// skips (y0, x0) and executes f on that cell last
void cellsMap(const int y0, const int x0, const int rows, const int cols, const int yCells[], const int xCells[], const int data[][cols], void (*f)(int, int, int, bool)) {
	// iterate up to y0 row
	for (int y = 0; y < y0; y++) {
		for (int x = 0; x < (cols + 1) / 2; x++) {
			f(yCells[y], xCells[x], data[y][x], false);
		}
		for (int x = (cols + 1) / 2; x < cols; x++) {
			f(yCells[y], xCells[x], data[y][x], true);
		}
	}

	// iterate up to either halfway or x0
	int x = 0;
	for (; x < x0 && x < (cols + 1) / 2; x++) {
		f(yCells[y0], xCells[x], data[y0][x], false);
	}

	// if stopped because reached x0, iterate until halfway
	// else if stopped because reached halfway, continue until reach x0
	// then continue
	if (x == x0) {
		x++;
		for (; x < (cols + 1) / 2; x++) {
			f(yCells[y0], xCells[x], data[y0][x], false);
		}
	}
	else {
		for (; x < x0; x++) {
			f(yCells[y0], xCells[x], data[y0][x], true);
		}
		x++;
	}
	for (; x < cols; x++) {
		f(yCells[y0], xCells[x], data[y0][x], true);
	}

	// iterate through the rest
	for (int y = y0 + 1; y < rows; y++) {
		for (int x = 0; x < (cols + 1) / 2; x++) {
			f(yCells[y], xCells[x], data[y][x], false);
		}
		for (int x = (cols + 1) / 2; x < cols; x++) {
			f(yCells[y], xCells[x], data[y][x], true);
		}
	}

	// finally deal with (y0, x0)
	attron(A_BOLD);
	f(yCells[y0], xCells[x0], (char)data[y0][x0], x0 >= (cols + 1) / 2);
	attroff(A_BOLD);
}

// draws the grid and draws initial set of numbers
// stores cell coordinates in yCells and xCells
void init(int rows, int cols, int data[][cols], int yCells[], int xCells[]) {
	// ncurses initialization 
	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);	
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			data[y][x] = y * rows + x;
		}
	}

	// get xlines and xcells
	int xLines[cols - 1];
	int xMargin = getMargin(COLS);
	const int lastLength = getCoords(COLS - 2 * xMargin, cols, xLines, xCells);
	int offsets[cols];
	getOffsets(cols - 1, lastLength, xCells, offsets);
	// the functions used return coordinates not relative to the edge of the screen
	// loop to add that in
	for (int i = 0; i < cols - 1; i++) {
		xLines[i] += xMargin;
		xCells[i] += xMargin;
	}
	xCells[cols - 1] += xMargin;

	// get ylines and ycells
	int yMargin = getMargin(LINES);
	int yLines[rows - 1];
	getCoords(LINES - 2 * yMargin, rows, yLines, yCells);

	for (int i = 0; i < rows - 1; i++) {
		yLines[i] += yMargin;
		yCells[i] += yMargin;
	}
	yCells[rows - 1] += yMargin;

	// print gridlines
	for (int i = 0; i < cols - 1; i++) {
		mvvline(yMargin, xLines[i], '|', LINES - 2 * yMargin);
	}
	for (int i = 0; i < rows - 1; i++) {
		mvhline(yLines[i], xMargin, '-', COLS - 2 * xMargin);
	}
	for (int x = 0; x < cols - 1; x++) {
		for (int y = 0; y < rows - 1; y++) {
			mvaddch(yLines[y], xLines[x], '+');
		}
	}

	cellsMap(0, 0, rows, cols, yCells, xCells, data, drawNum);
}

// swap and redraw two values at specified coords
void swap(int y, int x, int swapy, int swapx, int cols, int data[][cols], int yCoords[], int xCoords[]) {
	const bool xOnRight     = x     >= (cols + 1) / 2;
	const bool swapXOnRight = swapx >= (cols + 1) / 2;

	// clear out spaces on screen
	clearSpot(yCoords[y], xCoords[x], data[y][x], xOnRight);
	clearSpot(yCoords[swapy], xCoords[swapx], data[swapy][swapx], swapXOnRight);

	// redraw
	drawNum(yCoords[y], xCoords[x], data[swapy][swapx], xOnRight);
	attron(A_BOLD);
	drawNum(yCoords[swapy], xCoords[swapx], data[y][x], swapXOnRight);
	attroff(A_BOLD);

	// swap values in memory
	const int temp = data[y][x];
	data[y][x] = data[swapy][swapx];
	data[swapy][swapx] = temp;
}
