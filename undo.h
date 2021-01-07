#pragma once
#include <stdlib.h>

typedef struct Move {
	char move;
	struct Move *prev;
} Move;

void freeMoves(Move *move) {
	while (move != NULL) {
		const Move *prev = move->prev;
		free(move);
		move = prev;
	}
}
