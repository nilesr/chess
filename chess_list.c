#include <stdlib.h>
#include <assert.h>
#include "chess_list.h"

list_t* list_new() {
   return calloc(1, sizeof(list_t));
}
void list_free(list_t* list) {
   free(list);
}
void list_insert(list_t* list, move_t move) {
   assert(list->length < CHUNK_LEN);
   list->data[list->length++] = move;
}
