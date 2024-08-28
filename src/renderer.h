#include "mathc.h"

#ifndef RENDERER_H
#define RENDERER_H

void init_window(int width, int height);
void draw_container(mfloat_t* containerPos, int container);
void draw_particle(float x, float y, float radius);

#endif