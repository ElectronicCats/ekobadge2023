#ifndef LEVELS_H
#define LEVELS_H

#include "friend.h"

// Eko Party foundation year
#define FOUNDATION_YEAR 2001

#define SECRET_PHRASE "EC"
#define WINNER_BANNER "   L33t Hacker"

extern void Levels_Init();
extern uint16_t Get_Level();
extern void Set_Level(uint8_t newLevel);

#endif