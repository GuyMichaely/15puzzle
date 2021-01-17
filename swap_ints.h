#pragma once

void doNothing(int *a, int *b) {}
void swapInts(int *a, int *b) {
	const int temp = *a;
	*a = *b;
	*b = temp;
}
