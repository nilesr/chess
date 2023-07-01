#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "uci.h"

char* uci_move(move_t move, bool is_promo) {
   if (move.x1 == 0 && move.y1 == 0 && move.x2 == 0 && move.y2 == 0) {
      return strdup("0000");
   }
   char* buf = (char[20]) {};
   char* p = buf;
   *p++ = "abcdefgh"[move.y1];
   *p++ = "87654321"[move.x1];
   *p++ = "abcdefgh"[move.y2];
   *p++ = "87654321"[move.x2];
   if (is_promo) {
      *p++ = 'q';
   }
   *p++ = '\0';
   return strdup(buf);
}

move_t uci_extract_move(char* move) {
   if (strncmp(move, "(none)", strlen("(none)")) == 0) {
      return (move_t) {};
   }
   move_t result;
   result.y1 = move[0] - 'a';
   result.x1 = 7 - (move[1] - '1');
   result.y2 = move[2] - 'a';
   result.x2 = 7 - (move[3] - '1');
   return result;
}

stockfish_t stockfish_open(char* path) {
   char** argv = (char*[]) {path, NULL};
   stockfish_t engine = {};
   int pc[2];
   int cp[2];
   pipe(pc);
   pipe(cp);
   if (!(engine.child = fork())) {
      dup2(pc[0], 0);
      dup2(cp[1], 1);
      close(pc[0]);
      close(pc[1]);
      close(cp[0]);
      close(cp[1]);
      execve(path, argv, NULL);
   }
   close(pc[0]);
   close(cp[1]);
   engine.read_fd = cp[0];
   engine.write_fd = pc[1];
   dprintf(engine.write_fd, "setoption name UCI_LimitStrength value true\n");
   dprintf(engine.write_fd, "setoption name Skill Level value 0\n");
   dprintf(engine.write_fd, "uci\n");

   return engine;
}

move_t stockfish_play(stockfish_t* sf, move_t input, bool is_promo) {
   sf->move_history[sf->histptr++] = uci_move(input, is_promo);
   assert(sf->histptr < 1000);
   dprintf(sf->write_fd, "position startpos moves");
   for (int i = 0; i < sf->histptr; i++) {
      dprintf(sf->write_fd, " %s", sf->move_history[i]);
   }
   dprintf(sf->write_fd, "\n");
   dprintf(sf->write_fd, "go depth 1\n");
   char line[10240];
   int i = 0;
   char foo;
   int nread;
   while ((nread = read(sf->read_fd, &foo, 1)) != -1) {
      if (nread == 0) continue;
      assert(i < 10240);
      line[i++] = foo;
      if (foo == '\n') {
	 line[i] = 0;
	 if (strncmp(line, "bestmove ", strlen("bestmove ")) == 0) {
	    char* move = line + strlen("bestmove ");
	    move_t result = uci_extract_move(move);
	    if (move[4] == ' ' || move[4] == 0 || move[4] == '\n') {
	       move[4] = 0;
	    } else {
	       move[5] = 0;
	    }
	    sf->move_history[sf->histptr++] = strdup(move);
	    return result;
	 } else {
	    i = 0;
	 }
      }
   }
   abort();
}
