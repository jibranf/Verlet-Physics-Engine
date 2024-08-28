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

void applyContainerConstraints(int activeParticles, mfloat_t* containerPos, int container) {
    for (int i = 0; i < activeParticles; i++) {
        Particle* p = &(particles[i]);
        float responseFactor = 0.75;
        if (container == 0) {
            // handle box container collisions
            if (p->curr_position[0] < (containerPos[0] - CONTAINER_SIZE + CONTAINER_BORDER_WIDTH + p->radius)) {
                mfloat_t displacement = p->curr_position[0] - p->old_position[0];
                p->curr_position[0] = containerPos[0] - CONTAINER_SIZE + CONTAINER_BORDER_WIDTH + p->radius;
                p->old_position[0] = p->curr_position[0] + (displacement * responseFactor);
            }
            if (p->curr_position[0] > (containerPos[0] + CONTAINER_SIZE - CONTAINER_BORDER_WIDTH - p->radius)) {
                mfloat_t displacement = p->curr_position[0] - p->old_position[0];
                p->curr_position[0] = containerPos[0] + CONTAINER_SIZE - CONTAINER_BORDER_WIDTH - p->radius;
                p->old_position[0] = p->curr_position[0] + (displacement * responseFactor);
            }

            if (p->curr_position[1] < (containerPos[1] - CONTAINER_SIZE + CONTAINER_BORDER_WIDTH + p->radius)) {
                mfloat_t displacement = p->curr_position[1] - p->old_position[1];
                p->curr_position[1] = containerPos[1] - CONTAINER_SIZE + CONTAINER_BORDER_WIDTH + p->radius;
                p->old_position[1] = p->curr_position[1] + (displacement * responseFactor);
            }
            if (p->curr_position[1] > (containerPos[1] + CONTAINER_SIZE - CONTAINER_BORDER_WIDTH - p->radius)) {
                mfloat_t displacement = p->curr_position[1] - p->old_position[1];
                p->curr_position[1] = containerPos[1] + CONTAINER_SIZE - CONTAINER_BORDER_WIDTH - p->radius;
                p->old_position[1] = p->curr_position[1] + (displacement * responseFactor);
            }
        }
        if (container == 1) {
            // handle circle containere collisions
            mfloat_t displacement[VEC2_SIZE];
            vec2_subtract(displacement, p->curr_position, containerPos);
            mfloat_t dist = vec2_length(displacement);
            if (dist > CONTAINER_SIZE - p->radius) {
                mfloat_t normDisplacement[VEC2_SIZE];
                vec2_divide_f(normDisplacement, displacement, dist);
                vec2_multiply_f(normDisplacement, normDisplacement, CONTAINER_SIZE - p->radius);
                vec2_add(p->curr_position, containerPos, normDisplacement);
            }
        }
    }
}

void fixCollisions(Particle* p1, Particle* p2) {
    mfloat_t collision_axis[VEC2_SIZE];
    vec2_subtract(collision_axis, p1->curr_position, p2->curr_position);
    mfloat_t dist = vec2_length(collision_axis);
    if (dist < (p1->radius + p2->radius)) {
        mfloat_t norm[VEC2_SIZE];
        vec2_divide_f(norm, collision_axis, dist);
        mfloat_t delta = (p1->radius + p2->radius) - dist;
        vec2_multiply_f(norm, norm, 0.5 * 0.75 * delta);
        vec2_add(p1->curr_position, p1->curr_position, norm);
        vec2_subtract(p2->curr_position, p2->curr_position, norm);
    }
}

void detectCollisions(int activeParticles) {
    for (int i = 0; i < activeParticles; i++) {
        Particle* p1 = &(particles[i]);
        for (int j = 0; j < activeParticles; j++) {
            Particle *p2 = &(particles[j]);
            if (i != j) {
                fixCollisions(p1, p2);
            }
        }
    }
}