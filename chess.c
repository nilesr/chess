#include <stdatomic.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "chess.h"
#include "chess_list.h"
#include "uci.h"

static int* VALUES = (int[]) {/*empty*/ 0, /*K*/ 10000, /*Q*/ 90, /*R*/ 50, /*B*/ 35, /*N*/ 30, /*P*/ 10};
static char* NAMES = (char[]) {' ', 'K', 'Q', 'R', 'B', 'N', ' '};
static char COLORS[8*8] = {};
#ifdef NDEBUG
static const bool debug = false;
#else
static const bool debug = true;
#endif

mats_t count_mats(const char* board) {
   mats_t result = {};
   for (int i = 0; i < 8*8; i++) {
      char piece = board[i];
      if ((piece & 1) == WHITE) {
	 result.white += VALUES[piece >> 1];
      } else {
	 result.black += VALUES[piece >> 1];
      }
   }
   return result;
}

void init_board(char* board) {
   memset(board, 0, 8*8+1);
   char* p = board;
   *p++ = R | BLACK;
   *p++ = N | BLACK;
   *p++ = B | BLACK;
   *p++ = Q | BLACK;
   *p++ = K | BLACK;
   *p++ = B | BLACK;
   *p++ = N | BLACK;
   *p++ = R | BLACK;
   for (int i = 0; i < 8; i++) {
      *p++ = P | BLACK;
   }
   p += 8*4;
   for (int i = 0; i < 8; i++) {
      *p++ = P | WHITE;
   }
   *p++ = R | WHITE;
   *p++ = N | WHITE;
   *p++ = B | WHITE;
   *p++ = Q | WHITE;
   *p++ = K | WHITE;
   *p++ = B | WHITE;
   *p++ = N | WHITE;
   *p++ = R | WHITE;
}

char* make_board() {
   char* board = calloc(1, 8*8+1);
   init_board(board);
   return board;
}

char* acn_unapplied(const char* board, const move_t move) {
   char* buf = (char[20]) {};
   char* p = buf;
   char pchar = NAMES[board[move.x1*8+move.y1] >> 1];
   if (pchar != ' ') {
      *p++ = pchar;
   }
   *p++ = "abcdefgh"[move.y1];
   *p++ = "87654321"[move.x1];
   if (board[move.x2*8+move.y2]) {
      *p++ = 'x';
   }
   *p++ = "abcdefgh"[move.y2];
   *p++ = "87654321"[move.x2];
   *p++ = '\0';
   return strdup(buf);
}

void print_board(const char* board) {
   const char* fg = (char[]) {0x1b, 0x5b, 0x33, 0x38, 0x3b, 0x35, 0x3b, 0x31, 0x36, 0x6d, 0x00};
   const char* bg_black = (char[]) {   0x1b, 0x5b, 0x34, 0x38, 0x3b, 0x35, 0x3b, 0x31, 0x33, 0x37, 0x6d, 0x00};
   const char* bg_white = (char[]) {   0x1b, 0x5b, 0x34, 0x38, 0x3b, 0x35, 0x3b, 0x32, 0x32, 0x33, 0x6d, 0x00};
   const char* bg_green_black = (char[]) {   0x1b, 0x5b, 0x34, 0x38, 0x3b, 0x35, 0x3b, 0x31, 0x34, 0x32, 0x6d, 0x00};
   const char* bg_green_white = (char[]) {   0x1b, 0x5b, 0x34, 0x38, 0x3b, 0x35, 0x3b, 0x31, 0x38, 0x35, 0x6d, 0x00};
   const char* bg_red = (char[]) {   0x1b, 0x5b, 0x34, 0x38, 0x3b, 0x35, 0x3b, 0x31, 0x36, 0x30, 0x6d, 0x00};
   const char* reset = (char[]) {0x1b, 0x28, 0x42, 0x1b, 0x5b, 0x6d, 0x00};
   const char** white_symbols = (const char*[]) {" ", "♔", "♕", "♖", "♗", "♘", "♙"};
   const char** black_symbols = (const char*[]) {" ", "♚", "♛", "♜", "♝", "♞", "♟"};
   bool parity = true;
   printf(" ");
   for (int x = 0; x < 8; x++) {
      printf(" %c", "abcdefgh"[x]);
   }
   printf("\n");
   for (int x = 0; x < 8; x++) {
      printf("%d ", 8-x);
      printf("%s", fg);
      for (int y = 0; y < 8; y++) {
	 char piece = board[x*8+y];
	 char color_override = COLORS[x*8+y];
	 if (color_override) {
	    printf(color_override == GREEN ? (parity == WHITE ? bg_green_white : bg_green_black) : bg_red);
	 } else {
	    printf(parity ? bg_white : bg_black);
	 }
	 assert(piece >> 1 < 7);
	 printf(((piece & 1) == BLACK ? black_symbols : white_symbols)[piece >> 1]);
	 printf(" ");
	 parity = !parity;
      }
      parity = !parity;
      printf(reset);
      printf("\n");
   }
}
bool in_bounds(int x, int y) {
   return x >= 0 && x < 8 && y >= 0 && y < 8;
}
int sign(int i) {
   return i == 0 ? 0 : i < 0 ? -1 : 1;
}
bool is_piece(const char* board, const int x, const int y) {
   return in_bounds(x, y) && board[x*8+y];
}
bool is_piece_and_color(const char* board, const int x, const int y, const int color) {
   return in_bounds(x, y) && board[x*8+y] && (board[x*8+y] & 1) == color;
}
bool find_king(const char* board, int* kx, int* ky, const int color) {
   char* found = memchr(board, K | color, 8*8);
   if (!found) return false;
   int idx = found - board;
   *kx = idx / 8;
   *ky = idx % 8;
   return true;
}

bool could_threaten(int piece, int color, int dx, int dy) {
   piece &= 0b11111110;
   switch (piece) {
      case K:
	 return abs(dx) <= 1 && abs(dy) <= 1;
      case Q:
	 return true; // warning footgun
      case R:
	 return dx == 0 || dy == 0;
      case B:
	 return dx != 0 && dy != 0;
      case N:
	 return false; // warning also footgun
      case P:
	 if (color == WHITE) {
	    return dx == -1 && dy != 0;
	 } else {
	    return dx == 1 && dy != 0;
	 }
      default:
	 assert(false);
   }
}

bool is_promo(const char* board, const int x1, const int y1, const int x2, const int y2) {
   int piece = board[x1*8+y1] & 0b11111110;
   if (piece != P) {
      return false;
   }
   int color = board[x1*8+y1] & 1;
   if (color == WHITE) {
      return x2 == 0;
   } else {
      return x2 == 7;
   }

}
void cast_ignoring(const char* board, /* from */ int fx, int fy, /* ignored square */ int ix, int iy, /* not ignored square */ int nix, int niy, /* signs */ int sx, int sy, /* output */ int* ox, int* oy) {
   int i = 1;
   while (in_bounds(fx+sx*i, fy+sy*i)) {
      if (ix != nix || iy != niy) {
	 if (fx+sx*i == ix && fy+sy*i == iy) {
	    i += 1;
	    continue;
	 }
	 if (fx+sx*i == nix && fy+sy*i == niy) {
	    *ox = sx*i;
	    *oy = sy*i;
	    return;
	 }
      }
      if (!board[(fx+sx*i)*8+fy+sy*i]) {
	 i += 1;
	 continue;
      }
      *ox = sx*i;
      *oy = sy*i;
      return;
   }
   *ox = *oy = -100;
   return;
}

bool is_in_check_with_move(const char* board, const int kx, const int ky, const int x1, const int y1, const int x2, const int y2) {
   int cfx = x1 == kx && y1 == ky ? x2 : kx;
   int cfy = x1 == kx && y1 == ky ? y2 : ky;
   for (int sx = -1; sx <= 1; sx++) {
      for (int sy = -1; sy <= 1; sy++) {
	 if (sx == 0 && sy == 0) continue;
	 int dx, dy;
	 cast_ignoring(board, /* from */ cfx, cfy, /* ignoring */ x1, y1, /* not ignoring */ x2, y2, /* signs */ sx, sy, &dx, &dy);
	 if (dx < -8 || dy < -8) {
	    continue;
	 }
	 if (cfx + dx == x2 && cfy + dy == y2) {
	    // we're moving there, it'll be blocked
	    continue;
	 }
	 if ((board[(cfx+dx)*8+cfy+dy] & 1) == (board[kx*8+ky] & 1)) {
	    // it's our piece
	    continue;
	 }
	 if (could_threaten(board[(cfx+dx)*8+cfy+dy], board[(cfx+dx)*8+cfy+dy] & 1, -dx, -dy)) {
	    return true;
	 }
      }
   }
   int my_color = x1 == -1 ? (board[kx*8+ky] & 1) : (board[x1*8+y1] & 1);
   // Check for knights
   for (int i = -1; i <= 1; i += 2) {
      for (int j = -1; j <= 1; j += 2) {
	 if (!(x2 == cfx+i && y2 == cfy+2*j) && is_piece_and_color(board, cfx+i, cfy+2*j, !my_color) && (board[(cfx+i)*8+(cfy+2*j)] & 0b11111110) == N) {
	    return true;
	 }
	 if (!(x2 == cfx+2*i && y2 == cfy+j) && is_piece_and_color(board, cfx+2*i, cfy+j, !my_color) && (board[(cfx+2*i)*8+(cfy+j)] & 0b11111110) == N) {
	    return true;
	 }
      }
   }
   return false;
}

void check_and_add(list_t* out, const char* board, const mats_t* mats, const int x1, const int y1, const int x2, const int y2) {
   if (!in_bounds(x2, y2)) return;
   if (x1 == x2 && y1 == y2) {
      if (debug) COLORS[x2*8+y2] = RED;
      return;
   }
   int color = board[x1*8+y1] & 1;
   if (is_piece_and_color(board, x2, y2, color)) {
      if (debug) COLORS[x2*8+y2] = RED;
      return;
   }

   // check if this move jumps over another piece, except knights
   // TODO replace this whole section with a call to cast_ignoring
   if (x1 == x2) {
      int s = sign(y2-y1);
      for (int i = y1 + s; i != y2; i += s) {
	 if (board[x1*8+i]) {
	    // jumps piece
	    if (debug) COLORS[x2*8+y2] = RED;
	    return;
	 }
      }
   }
   if (y1 == y2) {
      int s = sign(x2-x1);
      for (int i = x1 + s; i != x2; i += s) {
	 if (board[i*8+y1]) {
	    // jumps piece
	    if (debug) COLORS[x2*8+y2] = RED;
	    return;
	 }
      }
   }
   if (abs(y2 - y1) == abs(x2 - x1)) {
      int s1 = sign(x2-x1);
      int s2 = sign(y2-y1);
      int ix, iy;
      for (ix = x1 + s1, iy = y1 + s2; ix != x2 && iy != y2; ix += s1, iy += s2) {
	 if (board[ix*8+iy]) {
	    // jumps piece
	    if (debug) COLORS[x2*8+y2] = RED;
	    return;
	 }
      }
   }
   // check if moving away from this square would put us in check
   // this could REALLY be memoized
   int kx, ky;
   bool found_king = find_king(board, &kx, &ky, color);
   assert(found_king);
   // skip checking if we are the king, this could be way more efficient
   if (kx != x1 || ky != y1) {
      int sx = sign(x1-kx);
      int sy = sign(y1-ky);
      int i = 1;
      bool found_self = false;
      // Cast until we find ourselves
      // TODO this could also be a cast_ignoring call
      while (in_bounds(kx+sx*i, ky+sy*i)) {
	 if (kx+sx*i == x1 && ky+sy*i == y1) {
	    found_self = true;
	    i += 1;
	    continue;
	 }
	 // If we find our *new* position before we find something else, we are safe
	 // For example, if you have a king below a pawn, an empty space, then an enemy rook,
	 // moving the pawn up one square into the empty space will not put us in check.
	 if (kx+sx*i == x2 && ky+sy*i == y2) {
	    break;
	 }
	 int piece = board[(kx+sx*i)*8+ky+sy*i];
	 if (!piece) {
	    i += 1;
	    continue;
	 }
	 // If we find something else first, we can skip the rest of the check
	 if (!found_self) {
	    break;
	 }
	 // We have found ourselves, and continued, and found another piece. If that piece can threaten in the direction
	 // of our king, we cannot move.
	 // ...Unless it's our piece
	 if ((piece & 1) == color) {
	    break;
	 }
	 if (could_threaten(piece, color, sx*i, sy*i)) {
	    if (debug) COLORS[x2*8+y2] = RED;
	    return;
	 } else {
	    break;
	 }
      }
   } else {
      // We are the king
      // check we are not *putting* ourself in check. This includes checking if we would be moving 1 square next to the other king
      if (is_in_check_with_move(board, x1, y1, x1, y1, x2, y2)) {
	 if (debug) COLORS[x2*8+y2] = RED;
	 return;
      }
   }
   // block if we are currently in check, and this would NOT get us out of it
   if (is_in_check_with_move(board, kx, ky, -1, -1, -1, -1) && is_in_check_with_move(board, kx, ky, x1, y1, x2, y2)) {
      if (debug) COLORS[x2*8+y2] = RED;
      return;
   }
   // block if we are taking a king, and it's *not* checkmate
   if (board[x2*8+y2] == (K | !(board[x1*8+y1] & 1))) {
      list_t* legal_moves = list_new();
      generate_all_moves(legal_moves, mats, board, !(board[x1*8+y1] & 1));
      if (legal_moves->length > 0) {
	 if (debug) COLORS[x2*8+y2] = RED;
	 list_free(legal_moves);
	 return;
      }
      printf("Enemy has no legal moves, it is a mate\n");
      list_free(legal_moves);
   }
   int promo = is_promo(board, x1, y1, x2, y2) ? VALUES[Q >> 1] - VALUES[P >> 1] : 0;
   int promo_white = (board[x2*8+y2] & 1) == WHITE ? promo : 0;
   int promo_black = (board[x2*8+y2] & 1) == BLACK ? promo : 0;
   int taken_white = (board[x2*8+y2] & 1) == WHITE ? VALUES[board[x2*8+y2] >> 1] : 0;
   int taken_black = (board[x2*8+y2] & 1) == BLACK ? VALUES[board[x2*8+y2] >> 1] : 0;
   list_insert(out, (move_t){x1, y1, x2, y2, {.white = mats->white - taken_white + promo_white, .black = mats->black - taken_black + promo_black}});
   if (debug) COLORS[x2*8+y2] = GREEN;
   return;
}
void generate_moves(list_t* out, const mats_t* mats, const char* board, const int x, const int y) {
   char piece = board[x*8+y];
   switch (piece & 0b11111110) {
      case K:
	 for (int i = -1; i <= 1; i++) {
	    for (int j = -1; j <= 1; j++) {
	       check_and_add(out, board, mats, x, y, x+i, y+j);
	    }
	 }
	 break;
      case Q:
	 for (int i = -8; i <= 8; i++) {
	    check_and_add(out, board, mats, x, y, x+i, y+i);
	    check_and_add(out, board, mats, x, y, x+i, y);
	    check_and_add(out, board, mats, x, y, x, y+i);
	    check_and_add(out, board, mats, x, y, x+i, y-i);
	 }
	 break;
      case R:
	 for (int i = -8; i <= 8; i++) {
	    check_and_add(out, board, mats, x, y, x+i, y);
	    check_and_add(out, board, mats, x, y, x, y+i);
	 }
	 break;
      case B:
	 for (int i = -8; i <= 8; i++) {
	    check_and_add(out, board, mats, x, y, x+i, y+i);
	    check_and_add(out, board, mats, x, y, x+i, y-i);
	 }
	 break;
      case N:
	 for (int i = -1; i <= 1; i += 2) {
	    for (int j = -1; j <= 1; j += 2) {
	       check_and_add(out, board, mats, x, y, x+i, y+2*j);
	       check_and_add(out, board, mats, x, y, x+2*i, y+j);
	    }
	 }
	 break;
      case P:
	 if ((piece & 1) == BLACK) {
	    if (!is_piece(board, x+1, y)) {
	       check_and_add(out, board, mats, x, y, x+1, y);
	    }
	    if (x == 1 && !is_piece(board, x+2, y)) {
	       check_and_add(out, board, mats, x, y, x+2, y);
	    }
	    if (is_piece_and_color(board, x+1, y+1, WHITE)) {
	       check_and_add(out, board, mats, x, y, x+1, y+1);
	    }
	    if (is_piece_and_color(board, x+1, y-1, WHITE)) {
	       check_and_add(out, board, mats, x, y, x+1, y-1);
	    }
	 } else {
	    if (!is_piece(board, x-1, y)) {
	       check_and_add(out, board, mats, x, y, x-1, y);
	    }
	    if (x == 6 && !is_piece(board, x-2, y)) {
	       check_and_add(out, board, mats, x, y, x-2, y);
	    }
	    if (is_piece_and_color(board, x-1, y+1, BLACK)) {
	       check_and_add(out, board, mats, x, y, x-1, y+1);
	    }
	    if (is_piece_and_color(board, x-1, y-1, BLACK)) {
	       check_and_add(out, board, mats, x, y, x-1, y-1);
	    }
	 }
	 break;
      default:
	 assert(false);
   }
}
void generate_all_moves(list_t* out, const mats_t* mats, const char* board, int side) {
   for (int x = 0; x < 8; x++) {
      for (int y = 0; y < 8; y++) {
	 if (board[x*8+y] && (board[x*8+y] & 0b00000001) == side) {
	    generate_moves(out, mats, board, x, y);
	 }
      }
   }
}

int white_advantage(const mats_t* m) {
   return m->white - m->black;
}
int side_advantage(const mats_t* m, int side) {
   return side == WHITE ? white_advantage(m) : -white_advantage(m);
}

void apply_move(char* board, move_t move) {
   // sometimes stockfish castles. Since it's stockfish doing it, I'm skipping
   // some checks about whether or not it's legal or overwriting a piece
   if (move.x1 == 0 && move.y1 == 4 && board[0*8+4] == (K | BLACK)) {
      if (move.x2 == 0 && move.y2 == 6 && board[0*8+7] == (R | BLACK)) {
	 board[0*8+5] = R | BLACK;
	 board[0*8+7] = 0;
      }
      if (move.x2 == 0 && move.y2 == 2 && board[0*8+0] == (R | BLACK)) {
	 board[0*8+3] = R | BLACK;
	 board[0*8+0] = 0;
      }
   }
   if (move.x1 == 7 && move.y1 == 4 && board[7*8+4] == (K | WHITE)) {
      if (move.x2 == 7 && move.y2 == 6 && board[7*8+7] == (R | WHITE)) {
	 board[7*8+5] = R | WHITE;
	 board[7*8+7] = 0;
      }
      if (move.x2 == 7 && move.y2 == 2 && board[7*8+0] == (R | WHITE)) {
	 board[7*8+3] = R | WHITE;
	 board[7*8+0] = 0;
      }
   }
   // handle en pissant
   if (move.x2 == (board[8*8] >> 3) && move.y2 == (board[8*8] & 0b111)) {
      if (move.x2 == 5 && (board[move.x1*8+move.y1] & 1) == BLACK) {
	 board[4*8+move.y2] = 0;
      }
      if (move.x2 == 2 && (board[move.x1*8+move.y1] & 1) == WHITE) {
	 board[3*8+move.y2] = 0;
      }
   }
   if (move.x1 == 6 && move.x2 == 4 && board[move.x1*8+move.y1] == (P | WHITE)) {
      board[8*8] = (5 << 3) + move.y1;
   } else if (move.x1 == 1 && move.x2 == 3 && board[move.x1*8+move.y1] == (P | BLACK)) {
      board[8*8] = (2 << 3) + move.y1;
   } else {
      board[8*8] = 0;
   }
   if (is_promo(board, move.x1, move.y1, move.x2, move.y2)) {
      board[move.x2*8+move.y2] = (board[move.x1*8+move.y1] & 1) | Q;
   } else {
      board[move.x2*8+move.y2] = board[move.x1*8+move.y1];
   }
   board[move.x1*8+move.y1] = 0;
}

move_t engine(const char* board, mats_t* mats, int color, int n) {
   list_t* l = list_new();
   generate_all_moves(l, mats, board, color);
   _Atomic uint64_t best_adv_move = (int64_t)INT32_MIN << 32;
   _Atomic int nseen = 1; // THIS IS NOT mathematically sound
   if (l->length == 0) return (move_t) {};
   mats_t* computed_mats = alloca(sizeof(mats_t) * l->length);
#pragma omp parallel for
   for (int node_idx = 0; node_idx < l->length; node_idx++) {
      move_t* move = l->data + node_idx;
      char* b2 = alloca(8*8+1);
      memcpy(b2, board, 8*8+1);
      apply_move(b2, *move);
      mats_t m2 = count_mats(b2);
      move_t response = {};
      if (n > 1) {
	 response = engine(b2, &m2, !color, n-1);
	 apply_move(b2, response);
	 //m2 = count_mats(b2);
      }
      int adv = side_advantage(&m2, color);
      if (n > 1 && response.x1 == 0 && response.y1 == 0 && response.x2 == 0 && response.y2 == 0) {
	 // We have a checkmate
	 // lol nevermind we may only have a stalemate
	 int ekx, eky;
	 bool found = find_king(b2, &ekx, &eky, !color);
	 if (!found || is_in_check_with_move(b2, ekx, eky, -1, -1, -1, -1 /* since response is already applied (and also it's empty) */)) {
	    adv = INT32_MAX;
	 }
      }
      int64_t bam_copy = atomic_load(&best_adv_move);
      int32_t current_best_adv = bam_copy >> 32;
      while (adv >= current_best_adv) {
	 // TODO not sure about this
	 if (current_best_adv == adv && (rand() % atomic_fetch_add_explicit(&nseen, 1, memory_order_relaxed)) != 0) {
	    break;
	 }
	 if (adv > current_best_adv) {
	    atomic_store_explicit(&nseen, 1, memory_order_relaxed);
	 }
	 int64_t new = (((int64_t)adv << 32) & 0xFFFFFFFF00000000) | (node_idx & 0xFFFFFFFF);
	 if (atomic_compare_exchange_weak_explicit(&best_adv_move, &bam_copy, new, memory_order_relaxed, memory_order_relaxed)) {
	    if (adv == INT32_MAX) {
	       computed_mats[node_idx] = (mats_t){.white = color == WHITE ? 20000 : 0, .black = color == BLACK ? 20000 : 0};
	    } else {
	       computed_mats[node_idx] = m2;
	    }
	    break;
	 }
	 current_best_adv = bam_copy >> 32;
      }
   }
   *mats = computed_mats[best_adv_move & 0xFFFFFFFF];
   move_t best_move = l->data[best_adv_move & 0xFFFFFFFF];
   list_free(l);
   return best_move;
}

void import_fen(char* board, int* color, char* pgn) {
   int i = 0;
   memset(board, 0, 8*8+1);
   while (*pgn && *pgn != ' ') {
      int color = (*pgn & 0b00100000) != 0 ? BLACK : WHITE;
      switch (*pgn & 0b11011111) {
	 case '/' & 0b11011111:
	    break;
	 case 'K':
	    board[i++] = K | color;
	    break;
	 case 'Q':
	    board[i++] = Q | color;
	    break;
	 case 'R':
	    board[i++] = R | color;
	    break;
	 case 'B':
	    board[i++] = B | color;
	    break;
	 case 'N':
	    board[i++] = N | color;
	    break;
	 case 'P':
	    board[i++] = P | color;
	    break;
	 case '1' & 0b11011111:
	 case '2' & 0b11011111:
	 case '3' & 0b11011111:
	 case '4' & 0b11011111:
	 case '5' & 0b11011111:
	 case '6' & 0b11011111:
	 case '7' & 0b11011111:
	 case '8' & 0b11011111:
	    i += *pgn - '0';
	    break;
	 default:
	    assert(false);
      }
      pgn++;
   }
   assert (*pgn); pgn++;
   *color = *pgn == 'w' ? WHITE : BLACK;
   assert (*pgn); pgn++;
   while (*pgn && *pgn != ' ') pgn++;
   if (*pgn && pgn[1] && *pgn != '-') {
      board[8*8] = ((*pgn - 'a') << 3) + (7 - (pgn[1] - '0'));
   }
}

void read_command(char* board, int* color) {
   char* line;
   size_t length = 0;
   size_t read = getline(&line, &length, stdin);
   if (read <= 0) abort();
   if (strncmp(line, "position fen ", strlen("position fen ")) == 0) {
      import_fen(board, color, line + strlen("position fen "));
   } else if (strncmp(line, "position startpos moves ", strlen("position startpos moves ")) == 0) {
      init_board(board);
      *color = WHITE;
      char* save = NULL;
      bool first = true;
      char* ucimove;
      while (ucimove = strtok_r(first ? line + strlen("position startpos moves ") : NULL, " \n", &save)) {
	 first = false;
	 move_t move = uci_extract_move(ucimove);
	 *color = !*color;
	 apply_move(board, move);
      }
   } else if (strncmp(line, "position startpos", strlen("position startpos")) == 0) {
      init_board(board);
      *color = WHITE;
   } else if (strncmp(line, "stop", strlen("stop")) == 0) {
      //
   } else if (strncmp(line, "quit", strlen("quit")) == 0) {
      exit(0);
   } else if (strncmp(line, "print", strlen("print")) == 0) {
      print_board(board);
   } else if (strncmp(line, "ucinewgame", strlen("ucinewgame")) == 0) {
      init_board(board);
      *color = WHITE;
   } else if (strncmp(line, "uci", strlen("uci")) == 0) {
      printf("id name chess\n");
      printf("id author Niles Rogoff niles.xyz github.com/nilesr\n");
      printf("\n");
      printf("uciok\n");
   } else if (strncmp(line, "isready", strlen("isready")) == 0) {
      printf("readyok\n");
   } else if (strncmp(line, "go", strlen("go")) == 0) {
      int depth = 5;
      char* save = NULL;
      bool first = true;
      char* option;
      int time = -1;
      bool time_next = false;
      bool depth_next = false;
      while (option = strtok_r(first ? line + strlen("go") : NULL, " \n", &save)) {
	 first = false;
	 if (strlen(option) == 0) continue;
	 if (time_next) {
	    time_next = false;
	    time = atoi(option);
	 }
	 if (strcmp(option, *color == WHITE ? "wtime" : "btime") == 0) {
	    time_next = true;
	 }
	 if (depth_next) {
	    depth_next = false;
	    depth = atoi(option);
	 }
	 if (strcmp(option, "depth") == 0) {
	    depth_next = true;
	 }
      }
      if (time >= 0) {
	 if (time > 10*60*1000) { // 10 minutes
	    depth = 6;
	 }
	 if (time < 15*1000) { // 15 seconds
	    depth = 4;
	 }
	 if (time < 5*1000) { // 5 seconds
	    depth = 3;
	 }
	 if (time < 1000) { // 1 second
	    depth = 1;
	 }
      }

      mats_t tm = count_mats(board);
      move_t result = engine(board, &tm, *color, depth);
      char* prettymove = uci_move(result, is_promo(board, result.x1, result.y1, result.x2, result.y2));
      printf("bestmove %s\n", prettymove);
      free(prettymove);
   } else {
      printf("Skipping unrecognized command: %s\n", line);
   }
   free(line);
   fflush(stdout);
}

int main(int argc, char** argv) {
   srand(time(NULL));
   int color = WHITE;
   char* board = make_board();
   while (argc > 1 && strncmp(argv[1], "--uci", strlen("--uci")) == 0) {
      read_command(board, &color);
   }
   stockfish_t sf = {};
   if (argc > 2 && strncmp(argv[1], "--stockfish", strlen("--stockfish")) == 0) {
      sf = stockfish_open(argv[2]);
   }

   int fullturn = 1;
   FILE* fd = fopen("game.pgn", "w");

   int wkx, wky, bkx, bky;
   move_t move;
   bool was_promo = false;
   while (find_king(board, &wkx, &wky, WHITE) && find_king(board, &bkx, &bky, BLACK)) {
      memset(COLORS, 0, 8*8);
      if (move.x1 || move.y1 || move.x2 || move.y2) {
	 COLORS[move.x1*8+move.y1] = GREEN;
	 COLORS[move.x2*8+move.y2] = GREEN;
      }
      if (is_in_check_with_move(board, wkx, wky, -1, -1, -1, -1)) {
	 COLORS[wkx*8+wky] = RED;
      }
      if (is_in_check_with_move(board, bkx, bky, -1, -1, -1, -1)) {
	 COLORS[bkx*8+bky] = RED;
      }
      print_board(board);
      struct timespec start, end;
      clock_gettime(CLOCK_MONOTONIC_RAW, &start);
      mats_t mats = count_mats(board);
      if (color == WHITE || !sf.child) {
	 move = engine(board, &mats, color, 5);
	 was_promo = is_promo(board, move.x1, move.y1, move.x2, move.y2);
      } else {
	 move = stockfish_play(&sf, move, was_promo);
      }
      clock_gettime(CLOCK_MONOTONIC_RAW, &end);
      if (move.x1 == 0 && move.y1 == 0 && move.x2 == 0 && move.y2 == 0) {
	 printf("Checkmate! %s wins!\n", color != WHITE ? "White" : "Black");
	 fprintf(fd, "%s\n", color != WHITE ? "1-0" : "0-1");
	 break;
      }
      char* nicemove = acn_unapplied(board, move);
      printf("%s moves %s, after %f seconds\n", color == WHITE ? "White" : "Black", nicemove, (end.tv_sec - start.tv_sec) + (float)(end.tv_nsec-start.tv_nsec)/1e9);
      if (color == WHITE) {
	 fprintf(fd, "%d. ", fullturn++);
      }
      fprintf(fd, "%s ", nicemove);
      free(nicemove);
      fflush(fd);
      apply_move(board, move);
      color = !color;
   }
}

