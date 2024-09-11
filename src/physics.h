#ifndef PHYSICS_H
#define PHYSICS_H

#include "mathc.h"

#define NUM_PARTICLES 5000
#define PARTICLE_RADIUS 3.0f
#define GRAVITY -1000.0f
#define CONTAINER_SIZE 400
#define CONTAINER_BORDER_WIDTH 1

// Grid-related definitions
#define CELL_SIZE (2 * PARTICLE_RADIUS)
#define GRID_WIDTH ((int)(CONTAINER_SIZE / CELL_SIZE) + 1)
#define GRID_HEIGHT ((int)(CONTAINER_SIZE / CELL_SIZE) + 1)
#define GRID_SIZE (GRID_WIDTH * GRID_HEIGHT)

typedef struct Particle {
    mfloat_t curr_position[VEC2_SIZE];
    mfloat_t old_position[VEC2_SIZE];
    mfloat_t acceleration[VEC2_SIZE];
    mfloat_t radius;
    struct Particle* next; // For linked list in grid cells
} Particle;

extern Particle* particles;
extern Particle** grid;

void updateParticlePositions(int activeParticles, float dt);
void applyGravity(int activeParticles);
void applyContainerConstraints(int activeParticles, mfloat_t* containerPos, int container);
void detectCollisions(int activeParticles);
void fixCollisions(Particle* p1, Particle* p2);

// New functions for spatial partitioning
void initGrid();
void updateGrid(int activeParticles);
int hashPosition(mfloat_t x, mfloat_t y);
void cleanupGrid();

#endif