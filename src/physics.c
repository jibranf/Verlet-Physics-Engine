#include "physics.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

Particle** grid = NULL;

void initGrid() {
    grid = (Particle**)malloc(GRID_SIZE * sizeof(Particle*));
    if (grid == NULL) {
        fprintf(stderr, "Failed to allocate memory for grid\n");
        exit(1);
    }
    for (int i = 0; i < GRID_SIZE; i++) {
        grid[i] = NULL;
    }
}

void cleanupGrid() {
    if (grid != NULL) {
        free(grid);
        grid = NULL;
    }
}

int hashPosition(mfloat_t x, mfloat_t y) {
    int gridX = (int)((x + CONTAINER_SIZE / 2) / CELL_SIZE);
    int gridY = (int)((y + CONTAINER_SIZE / 2) / CELL_SIZE);
    return gridY * GRID_WIDTH + gridX;
}

void updateGrid(int activeParticles) {
    // Clear the grid
    for (int i = 0; i < GRID_SIZE; i++) {
        grid[i] = NULL;
    }

    // Add particles to the grid
    for (int i = 0; i < activeParticles; i++) {
        Particle* p = &particles[i];
        int hash = hashPosition(p->curr_position[0], p->curr_position[1]);
        if (hash >= 0 && hash < GRID_SIZE) {
            p->next = grid[hash];
            grid[hash] = p;
        }
    }
}

void updateParticlePositions(int activeParticles, float dt) {
    float damping = 0.999f; // Damping factor (close to 1 means low damping)
    for (int i = 0; i < activeParticles; i++) {
        Particle *p = &(particles[i]);
        mfloat_t velocity[VEC2_SIZE];
        vec2_subtract(velocity, p->curr_position, p->old_position);
        vec2_multiply_f(velocity, velocity, damping); // Apply damping to velocity
        vec2_assign(p->old_position, p->curr_position);
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
        float responseFactor = 0.45;
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
            // handle circle container collisions
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
    const float epsilon = 0.001f;  // Small threshold to avoid jitter
    if (dist < (p1->radius + p2->radius) - epsilon) {
        mfloat_t norm[VEC2_SIZE];
        vec2_divide_f(norm, collision_axis, dist);
        mfloat_t delta = 0.5 * 0.75 * ((p1->radius + p2->radius) - dist);
        vec2_multiply_f(norm, norm, delta);
        vec2_add(p1->curr_position, p1->curr_position, norm);
        vec2_subtract(p2->curr_position, p2->curr_position, norm);
    }
}

void detectCollisions(int activeParticles) {
    updateGrid(activeParticles);

    for (int i = 0; i < GRID_SIZE; i++) {
        Particle* p1 = grid[i];
        while (p1 != NULL) {
            // Check collisions within the same cell
            Particle* p2 = p1->next;
            while (p2 != NULL) {
                fixCollisions(p1, p2);
                p2 = p2->next;
            }

            // Check collisions with adjacent cells
            int x = i % GRID_WIDTH;
            int y = i / GRID_WIDTH;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                        int neighborIndex = ny * GRID_WIDTH + nx;
                        Particle* neighbor = grid[neighborIndex];
                        while (neighbor != NULL) {
                            fixCollisions(p1, neighbor);
                            neighbor = neighbor->next;
                        }
                    }
                }
            }

            p1 = p1->next;
        }
    }
}