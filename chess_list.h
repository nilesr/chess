#pragma once

#include "chess.h"

list_t* list_new();
void list_free(list_t* list);
void list_insert(list_t* list, move_t move);
