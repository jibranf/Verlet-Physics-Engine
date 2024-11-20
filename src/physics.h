#ifndef PHYSICS_H
#define PHYSICS_H

#include "mathc.h"
#include "renderer.h"

#define NUM_PARTICLES 5000
#define PARTICLE_RADIUS 4.0f
#define GRAVITY -981.0f
#define CONTAINER_SIZE 400
#define CONTAINER_BORDER_WIDTH 0

#define GRID_CELL_SIZE (2 * PARTICLE_RADIUS)
#define GRID_WIDTH ((int)(WINDOW_WIDTH / GRID_CELL_SIZE) + 2)
#define GRID_HEIGHT ((int)(WINDOW_HEIGHT / GRID_CELL_SIZE) + 2)
#define MAX_PARTICLES_PER_CELL 100 // Adjust as necessary

typedef struct {
    mfloat_t curr_position[VEC2_SIZE];
    mfloat_t old_position[VEC2_SIZE];
    mfloat_t acceleration[VEC2_SIZE];
    mfloat_t radius;
} Particle;

typedef struct {
    int num_particles;
    int particle_indices[MAX_PARTICLES_PER_CELL];
} GridCell;

extern Particle particles[NUM_PARTICLES];

void updateParticlePositions(int activeParticles, float dt);
void applyGravity(int activeParticles);
void applyContainerConstraints(int activeParticles, mfloat_t* containerPos, int container);
void detectCollisions(int activeParticles);
void fixCollisions(Particle* p1, Particle* p2);

#endif