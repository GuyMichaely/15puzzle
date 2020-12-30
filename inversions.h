#include <stdbool.h>

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

// count inversions in array while performing merge sort
// end is index after last element
// modifies the input array
// returns false for even parity false otherwise
bool mergeSortInversions(int start[], int end[]) {
	if (end - start == 1) {
		return 0;
	}
	
	bool inversions = false;
	int *sep = start + (end - start) / 2; // seperation between the two list halves
	inversions += mergeSortInversions(start, sep);
	inversions ^= mergeSortInversions(sep, end);

	while (end != sep && sep != start) {
		if (start[0] > sep[0]) {
			// number of elements in second list less than first element of first list
			int numSmaller = firstGreaterIndex(start[0], sep, end) - sep;
			inversions ^= (numSmaller * (sep - start)) % 2;
			
			// save the numbers we're about to overwrite
			int temps[numSmaller];
			for (int i = 0; i < numSmaller; i++) {
				temps[i] = sep[i];
			}
			
			// shift the numbers in the first list by numSmaller
			for (int *i = sep - 1; i >= start; i--) {
				i[numSmaller] = *i;
			}
			sep += numSmaller;

			// put the smaller numbers at the beginning
			for (int i = 0; i < numSmaller; i++) {
				start[i] = temps[i];
			}
			start += numSmaller;
		}
		start++;
	}
	
	return inversions;
}
