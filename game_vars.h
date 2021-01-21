#pragma once

#include "undo.h"
typedef struct GameVars {
	int *cells;
	int *yCoords;
	int *xCoords;
	int rows;
	int cols;
	int y;
	int x;
	Move *undo;
	int* coordinates;
} GameVars;

int getV(GameVars *game, int y, int x) {
	return game->cells[y * game->cols + x];
}

void setV(GameVars *game, int y, int x, int v) {
	game->cells[y * game->cols + x] = v;
}
