#include "physics.h"
#include <stdlib.h>
#include <stdio.h>

Particle particles[NUM_PARTICLES];
static GridCell grid[GRID_WIDTH][GRID_HEIGHT];

void updateParticlePositions(int activeParticles, float dt) {
    for (int i = 0; i < activeParticles; i++) {
        Particle *p = &(particles[i]);
        mfloat_t velocity[VEC2_SIZE];
        vec2_subtract(velocity, p->curr_position, p->old_position);
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
        float responseFactor = 0.75;
        if (container == 0) {
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
    // Clear grid cells
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            grid[i][j].num_particles = 0;
        }
    }

    // Assign particles to grid cells
    for (int p_idx = 0; p_idx < activeParticles; p_idx++) {
        Particle* p = &particles[p_idx];

        // Compute cell indices
        int cell_x = (int)(p->curr_position[0] / GRID_CELL_SIZE);
        int cell_y = (int)(p->curr_position[1] / GRID_CELL_SIZE);

        // Ensure indices are within grid bounds
        if (cell_x < 0) cell_x = 0;
        else if (cell_x >= GRID_WIDTH) cell_x = GRID_WIDTH - 1;

        if (cell_y < 0) cell_y = 0;
        else if (cell_y >= GRID_HEIGHT) cell_y = GRID_HEIGHT - 1;

        // Add particle to grid cell
        GridCell* cell = &grid[cell_x][cell_y];
        if (cell->num_particles < MAX_PARTICLES_PER_CELL) {
            cell->particle_indices[cell->num_particles++] = p_idx;
        } else {
            // Handle error: too many particles in cell
            printf("Error: too many particles in cell (%d, %d)\n", cell_x, cell_y);
        }
    }

    // Collision detection using grid
    for (int i = 0; i < GRID_WIDTH; i++) {
        for (int j = 0; j < GRID_HEIGHT; j++) {
            GridCell* cell = &grid[i][j];
            for (int idx1 = 0; idx1 < cell->num_particles; idx1++) {
                int p_idx1 = cell->particle_indices[idx1];
                Particle* p1 = &particles[p_idx1];

                // Check collisions in same and neighboring cells
                for (int di = -1; di <= 1; di++) {
                    int ni = i + di;
                    if (ni < 0 || ni >= GRID_WIDTH) continue;
                    for (int dj = -1; dj <= 1; dj++) {
                        int nj = j + dj;
                        if (nj < 0 || nj >= GRID_HEIGHT) continue;

                        GridCell* neighbor_cell = &grid[ni][nj];
                        for (int idx2 = 0; idx2 < neighbor_cell->num_particles; idx2++) {
                            int p_idx2 = neighbor_cell->particle_indices[idx2];
                            if (p_idx2 <= p_idx1) continue; // avoid double checking and self-check

                            Particle* p2 = &particles[p_idx2];
                            fixCollisions(p1, p2);
                        }
                    }
                }
            }
        }
    }
}
