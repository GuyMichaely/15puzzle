#include <ncurses.h>
#include <unistd.h>

#include "drawing.h"

static inline void clearMsg(const int y, char *msg) {
	mvhline(y, (COLS - strlen(msg)) / 2, ' ', strlen(msg));
}

static inline void clearMsgs() {
	nodelay(stdscr, true); // causes getch() to be non blocking
	clearMsg(0, "What algorithm should be used to solve?");
	clearMsg(1, "0: Just for fun inefficient algorithm");
	clearMsg(2, "1: A* with linear conflict + manhattan distance as heuristic");
	clearMsg(3, "2: https://www.aaai.org/Papers/JAIR/Vol22/JAIR-2209.pdf");

}
void funAi(const int cols, const int rows, int *y0, int *x0, int cells[][cols], int xCoords[], int yCoords[]) {
	// bring 0 to (0, 0)
	swap0(y0, x0, *y0, *x0 - 1, cols, cells, xCoords, yCoords);
	while (*x0) {
		usleep(400000);
		const int c = getch();
		if (c == 'q' || c == 'Q') {
			return;
		}
		swap0(y0, x0, *y0, *x0 - 1, cols, cells, xCoords, yCoords);
	}
	while (*y0) {
		usleep(400000);
		const int c = getch();
		if (c == 'q' || c == 'Q') {
			return;
		}
		swap0(y0, x0, *y0 - 1, *x0, cols, cells, xCoords, yCoords);
	}
}

void ai(const int cols, const int rows, int *y0, int *x0, int cells[][cols], int xCoords[], int yCoords[]) {
	midPrint(0, "What algorithm should be used to solve?");
	midPrint(1, "0: Just for fun inefficient algorithm");
	midPrint(2, "1: A* with linear conflict + manhattan distance as heuristic");
	midPrint(3, "2: https://www.aaai.org/Papers/JAIR/Vol22/JAIR-2209.pdf");
	int c;

	while ((c = getch()) != 'c' && c != 'C') {
		switch (c) {
			case 'q':
			case 'Q':
				endwin();
				exit(0);
			case '0':
				clearMsgs();
				funAi(cols, rows, y0, x0, cells, xCoords, yCoords);
				return;
		}
	}
	clearMsgs();
	return;
}
