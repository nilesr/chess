#pragma once

#define WHITE 0
#define BLACK 1
#define K 0b0010
#define Q 0b0100
#define R 0b0110
#define B 0b1000
#define N 0b1010
#define P 0b1100

#define RED   1
#define GREEN 2

typedef struct mats_t {
	int white;
	int black;
} mats_t;
typedef struct move_t {
	char x1, y1, x2, y2;
	mats_t mats;
} move_t;

#define CHUNK_LEN 10000
typedef struct list_t {
	move_t data[CHUNK_LEN];
	int length;
} list_t;

void generate_all_moves(list_t* out, const mats_t* mats, const char* board, int side);
