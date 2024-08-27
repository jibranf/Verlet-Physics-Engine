#include "physics.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

Particle particles[NUM_PARTICLES];

void updateParticlePositions(int activeParticles, float dt) {
    for (int i = 0; i < activeParticles; i++) {
        Particle *p = &(particles[i]);
        mfloat_t velocity[VEC2_SIZE];
        vec2_subtract(velocity, p->curr_position, p->old_position);
        vec2_assign(p->old_position, p->curr_position); // update old position to current position
        vec2_multiply_f(p->acceleration, p->acceleration, dt * dt);
        vec2_add(p->curr_position, p->curr_position, velocity);
        vec2_add(p->curr_position, p->curr_position, p->acceleration);
        vec2_zero(p->acceleration);
    }
}

void applyGravity(int activeParticles) {
    for (int i = 0; i < activeParticles; i++) {
        particles[i].acceleration[1] += GRAVITY;
    }
}

