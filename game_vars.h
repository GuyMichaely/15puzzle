#pragma once

#include "undo.h"
typedef struct GameVars {
	int **cells;
	int *yCoords;
	int *xCoords;
	int rows;
	int cols;
	int y;
	int x;
	Move *undo;
	int* coordinates;
} GameVars;
