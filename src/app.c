#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include "renderer.h"
#include "physics.h"
#include <time.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define PARTICLE_SPAWN_X WINDOW_WIDTH/3
#define PARTICLE_SPAWN_Y WINDOW_HEIGHT/3

#define TARGET_FPS 60.0
#define SPAWN_DELAY 0.01

#define SUBSTEPS 8

#define CONTAINER 0 // box = 0, circle = 1

int elapsedFrames = 0;

void instantiateParticles(Particle* particle_list, int numParticles) {
    for (int i = 0; i < numParticles; i++) {
        Particle* p = &(particle_list[i]);

        // ====== DEFAULT ======
        // mfloat_t x = PARTICLE_SPAWN_X + 300;
        // mfloat_t y = PARTICLE_SPAWN_Y - 100;
        // vec2(p->curr_position, x, y);
        // vec2(p->old_position, x, y);
        // vec2(p->acceleration, 0, 0);
        // p->radius = PARTICLE_RADIUS;        

        // ====== DEFAULT ALTERNATING ===========
        // mfloat_t x = PARTICLE_SPAWN_X;
        // mfloat_t y = PARTICLE_SPAWN_Y;
        // if (i % 2 == 0) {
        //     x += 0.25 * CONTAINER_SIZE;
        // } else {
        //     x -= 0.25 * CONTAINER_SIZE;
        // }
        // vec2(p->curr_position, x, y);
        // vec2(p->old_position, x, y);
        // vec2(p->acceleration, 0, 0);
        // p->radius = PARTICLE_RADIUS;

        // ===== STREAM =====
        int distance = 7.0f;
        mfloat_t x = PARTICLE_SPAWN_X + ((i) % distance - distance / 2);
        mfloat_t y = PARTICLE_SPAWN_Y + 4.0f;
        mfloat_t xp = x * 0.995;
        mfloat_t yp = y * 0.998;
        vec2(p->curr_position, x, y);
        vec2(p->old_position, xp, yp);
        vec2(p->acceleration, 0, 0);
        p->radius = PARTICLE_RADIUS;
    }
}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Verlet Physics Engine", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    init_window(WINDOW_WIDTH, WINDOW_HEIGHT);
    mfloat_t containerPos[VEC2_SIZE] = {WINDOW_WIDTH/2, WINDOW_HEIGHT/2};

    instantiateParticles(particles, NUM_PARTICLES);
    int activeParticles = 0;
    float spawnTimer = 0.0;

    float dt = 0.000001f;
    float lastFrameTime = (float)glfwGetTime();
    char title[100] = "";
    srand(time(NULL));

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // spawning particles while sim is at or above target framerate
        spawnTimer += dt;
        if (1.0 / dt >= TARGET_FPS - 0.1 && spawnTimer >= SPAWN_DELAY && activeParticles < NUM_PARTICLES) {
            activeParticles += 1;
            spawnTimer = 0.0;
        }
        
        if (elapsedFrames % 60 == 0) {
            sprintf(title, "FPS : %-4.0f | Particles : %-10d", 1.0 / dt, activeParticles);
            glfwSetWindowTitle(window, title);
        }

        // update its physics
        float sub_dt = dt / SUBSTEPS;
        for (int i = 0; i < SUBSTEPS; i++) {
            applyGravity(activeParticles);
            applyContainerConstraints(activeParticles, containerPos, CONTAINER);
            detectCollisions(activeParticles);
            updateParticlePositions(activeParticles, sub_dt);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        // drawing container and each particle
        draw_container(containerPos, CONTAINER);
        for (int i = 0; i < activeParticles; i++) {
            draw_particle(particles[i].curr_position[0], particles[i].curr_position[1], particles[i].radius);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        dt = (float)glfwGetTime() - lastFrameTime;
        while (dt < 1.0f / TARGET_FPS) {
            dt = (float)glfwGetTime() - lastFrameTime;
        }
        lastFrameTime = (float)glfwGetTime();
    }

    glfwTerminate();
    return 0;
}