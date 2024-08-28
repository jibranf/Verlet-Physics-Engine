#ifndef PHYSICS_H
#define PHYSICS_H

#include "mathc.h"

#define NUM_PARTICLES 500
#define PARTICLE_RADIUS 5.0f
#define GRAVITY 1000.0f
#define CONTAINER_SIZE 500
#define CONTAINER_BORDER_WIDTH 1

typedef struct {
    mfloat_t curr_position[VEC2_SIZE];
    mfloat_t old_position[VEC2_SIZE];
    mfloat_t acceleration[VEC2_SIZE];
    mfloat_t radius;
} Particle;


extern Particle particles[NUM_PARTICLES];

void updateParticlePositions(int activeParticles, float dt);
void applyGravity(int activeParticles);
void applyContainerConstraints(int activeParticles, mfloat_t* containerPos, int container);
void detectCollisions(int activeParticles);
void fixCollisions(Particle* p1, Particle* p2);

#endif