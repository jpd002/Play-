#ifndef _GUNCON2_UTILS_H
#define _GUNCON2_UTILS_H
#include "PS2VM.h"

const struct Iop::LightgunInfo* GetLightgunInfo(const char* gameName);
void GunCon2SetState(CPS2VM* vm, int instance, uint32 buttons, float x, float y);
#endif