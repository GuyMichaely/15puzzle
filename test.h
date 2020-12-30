#define clearPrint(y, x, ...) \
do { \
	mvhline((y), (x), ' ', 69); \
	mvprintw((y), (x), __VA_ARGS__); \
} while (false)

#define interactiveDebug(...) \
do { \
	mvhline(0, 0, ' ', 69); \
	mvprintw(0, 0, __VA_ARGS__); \
	if (getch() == 'q') { \
		endwin(); \
		exit(0); \
	} \
} while (false)

void ncursesPrintArr(int y, int x, int myArr[], int length) {
    int spaces = 0;
    for (int i = 0; i < length; i++) {
        mvprintw(y, x + spaces + i, "%i", myArr[i]);
	spaces += intLength(myArr[i]);
    }
}
 void quitGetch() {
 	int c = getch();
	if (c == 'q' || c == 'Q') {
		endwin();
		exit(0);
	}
}
