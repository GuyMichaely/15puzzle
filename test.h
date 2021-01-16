#pragma once

#define clearPrinsplitt(y, x, ...) \
do { \
	mvhline((y), (x), ' ', 69); \
	mvprintw((y), (x), __VA_ARGS__); \
} while (false)

 void quitGetch() {
 	int c = getch();
	if (c == 'q' || c == 'Q') {
		endwin();
		exit(0);
	}
}

void floatGetch(float f) {
	mvhline(0, 0, ' ', 99);
	mvprintw(0, 0, "%f", f);
	quitGetch();
}

#define interactiveDebug(...) \
do { \
	mvhline(0, 0, ' ', 69); \
	mvprintw(0, 0, __VA_ARGS__); \
	quitGetch(); \
} while (false);

void printArr(int y, int x, int myArr[], int length) {
    mvhline(y, x, ' ', 225);
    int spaces = 0;
    for (int i = 0; i < length; i++) {
        mvprintw(y, x + spaces + i, "%i", myArr[i]);
	spaces += intLength(myArr[i]);
    }
}

void getchPrintArr(int y, int x, int myArr[], int length) {
	printArr(y, x, myArr, length);
	quitGetch();
}
