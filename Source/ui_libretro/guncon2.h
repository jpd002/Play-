#ifndef _GUNCON2_H
#define _GUNCON2_H
#include "PS2VM.h"

void register_guns(CPS2VM* vm);
void set_gun(unsigned port, bool isGun);
void update_guns(CPS2VM* vm);
#endif