#include <stdbool.h>
#include <string.h>
#include "test.h"
// return index of smallest number bigger than v
// or returns end if v larger than all
// end is index after last element
int *firstGreaterIndex(int v, int start[], int end[]) {
	while (start != end) {
		const int *mid = start + (end - start) / 2;
		if (*mid > v) {
			end = mid;
		}
		else {
			start = mid + 1;
		}
	}
	return start;
}

// credit for this algorithm goes to Chris Calabro
// http://cseweb.ucsd.edu/~ccalabro/essays/15_puzzle.pdf
// gives sign of inversion in O(length) iterations
bool inversionParity(int arr[], int length) {
        int copy[length];
	// getchPrintArr(0, 0, arr, length);
        memcpy(copy, arr, length * sizeof(int));
        bool s = false;
        int i = 0;
        while (i < length) {
                if (i != copy[i]) {
			// swap copy[i] and copy[copy[i]]
                        const int temp = copy[i];
                        copy[i] = copy[temp];
                        copy[temp] = temp;

                        s = !s; 
                }   
                else {
                        i++;
                }   
        }
        return s;
}
