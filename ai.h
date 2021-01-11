#include <ncurses.h>
#include <unistd.h>

#include "drawing.h"

#include "undo.h"
#include "globals.h"

#define MOVE_DELAY 250000
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

// sleeps and returns getch() == 'q' or 'Q'
bool sleepGetch() {
	usleep(MOVE_DELAY);
	const int c = getch();
	if (c == 'q' || c == 'Q') {
		nodelay(stdscr, false);
		return true;
	}
	return false;
}

// void move(int srcy, int srcx, const int desty, const int destx) {
//         if (){}
// }

void funAiVert(Move **undo, const int lessY, const int lessX) {
	for(;;){} 
}

void funAi(Move **undo) {
	
}

void ai(Move **undo) {
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
				funAi(undo);
				return;
		}
	}
	clearMsgs();
	return;
}
