#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include "renderer.h"
#include "physics.h"
#include <time.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define PARTICLE_SPAWN_X WINDOW_WIDTH/2
#define PARTICLE_SPAWN_Y WINDOW_HEIGHT/2

#define TARGET_FPS 60.0
#define SPAWN_DELAY 0.1

#define SUBSTEPS 8

#define CONTAINER 1 // box = 0, circle = 1

int totalFrames = 0;

void instantiateParticles(Particle* particle_list, int numParticles);

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


    instantiateParticles(particles, NUM_PARTICLES);
    int activeParticles = 0;
    init_window(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    float dt = 0.000001f;
    float lastFrameTime = (float)glfwGetTime();
    char title[100] = "";
    srand(time(NULL));

    float lastTime = (float)glfwGetTime();
    float spawnTimer = 0.0;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // TODO: figure out how to consolidate this with current dt calculation at bottom
        float currTime = (float)glfwGetTime();
        float deltaTime = currTime - lastTime;
        lastTime = currTime;
        spawnTimer += deltaTime;
        if (spawnTimer >= SPAWN_DELAY && activeParticles < NUM_PARTICLES) {
            activeParticles += 1; // spawn a particle
            spawnTimer = 0.0;
        }
        // if (1.0 / dt >= TARGET_FPS - 5 && glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && activeParticles < NUM_PARTICLES) {
        //     activeParticles += 1;
        // }
        
        // update its physics
        float sub_dt = dt / SUBSTEPS;
        for (int i = 0; i < SUBSTEPS; i++) {
            applyGravity(activeParticles);
            applyContainerConstraints(activeParticles, WINDOW_WIDTH, WINDOW_HEIGHT);
            detectCollisions(activeParticles);
            updateParticlePositions(activeParticles, sub_dt);
        }

        glClear(GL_COLOR_BUFFER_BIT);

        render_scene(CONTAINER, activeParticles);

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

void instantiateParticles(Particle* particle_list, int numParticles) {
    int distance = 15.0f;

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
        mfloat_t x = PARTICLE_SPAWN_X;
        mfloat_t y = PARTICLE_SPAWN_Y;
        if (i % 2 == 0) {
            x += 200;
        } else {
            x -= 200;
        }
        vec2(p->curr_position, x, y);
        vec2(p->old_position, x, y);
        vec2(p->acceleration, 0, 0);
        p->radius = PARTICLE_RADIUS;
    }
}