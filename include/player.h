#ifndef PLAYER_H
#define PLAYER_H

#include "game.h"
#include "stage.h"

void init_player(Player *p, const Stage *stage);
void move_player(Player *p, char input, const Stage *stage);

#endif
