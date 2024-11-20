#ifndef RENDERER_H
#define RENDERER_H

#include "mathc.h"

#define WINDOW_WIDTH 1536
#define WINDOW_HEIGHT 864

void draw_container(mfloat_t* containerPos, int container);

// Initializes the renderer with the given window dimensions
void init_renderer(int window_width, int window_height);

// Draws particles using point primitives
// activeParticles: Number of active particles to render
// positions: Array of positions (x, y) for each active particle
void draw_particles(int activeParticles, float* data, bool colorByVelocity);

// Cleans up renderer resources
void cleanup_renderer();

// Updates the projection matrix (useful if window is resized)
void update_projection(int window_width, int window_height);

#endif