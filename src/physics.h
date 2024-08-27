#ifndef PHYSICS_H
#define PHYSICS_H

#include "mathc.h"

#define NUM_PARTICLES 10
#define PARTICLE_RADIUS 25.0f
#define GRAVITY 1000.0f

typedef struct {
    mfloat_t curr_position[VEC2_SIZE];
    mfloat_t old_position[VEC2_SIZE];
    mfloat_t acceleration[VEC2_SIZE];
    mfloat_t radius;
} Particle;


extern Particle particles[NUM_PARTICLES];

void updateParticlePositions(int activeParticles, float dt);
void applyGravity(int activeParticles);


#endif