#pragma once

#include "chess.h"

typedef struct stockfish_t {
	int read_fd;
	int write_fd;
	pid_t child;
	char* move_history[1000];
	int histptr;
} stockfish_t;

char* uci_move(move_t move, bool is_promo);
move_t uci_extract_move(char* move);

stockfish_t stockfish_open(char* path);

move_t stockfish_play(stockfish_t* sf, move_t input, bool is_promo);

