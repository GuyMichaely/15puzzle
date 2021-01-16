#pragma once

// returns b for use in ai.h, elsewhere is useless
int *swapInts(int *a, int *b) {
	const int temp = *a;
	*a = *b;
	*b = temp;
	return b;
}
