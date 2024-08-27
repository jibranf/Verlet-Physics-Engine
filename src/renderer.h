#ifndef RENDERER_H
#define RENDERER_H

void init_window(int width, int height);
void render_scene(int container, int activeParticles);
void draw_container(int container);
void draw_particle(float x, float y);

#endif