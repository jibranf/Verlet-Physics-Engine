#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "renderer.h"
#include "physics.h"
#include <time.h>
#include <string.h>

#define PARTICLE_SPAWN_X (WINDOW_WIDTH / 4)
#define PARTICLE_SPAWN_Y (WINDOW_HEIGHT * 0.99f)

#define TARGET_FPS 60.0
#define SPAWN_DELAY 0.01

#define SUBSTEPS 8

#define CONTAINER 0 // box = 0, circle = 1

int elapsedFrames = 0;

extern Particle particles[NUM_PARTICLES];

void instantiateParticles(Particle* particle_list, int numParticles) {
    for (int i = 0; i < numParticles; i++) {
        Particle* p = &(particle_list[i]);
        // ===== STREAM =====
        int distance = 7.0f;
        mfloat_t x = PARTICLE_SPAWN_X + ((i) % distance - distance / 2);
        mfloat_t y = PARTICLE_SPAWN_Y;
        mfloat_t xp = x * 0.995;
        mfloat_t yp = y * 0.998;
        vec2(p->curr_position, x, y);
        vec2(p->old_position, xp, yp);
        vec2(p->acceleration, 0, 0);
        p->radius = PARTICLE_RADIUS;
    }
}

void update_projection(int window_width, int window_height);

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    update_projection(width, height);
}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); 

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Verlet Physics Engine", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW: %s\n", glewGetErrorString(err));
        glfwTerminate();
        return -1;
    }

    glClearColor(0.10, 0.10, 0.10, 1.0);
    glClearStencil(0);

    // Enable blending for transparency
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    init_renderer(WINDOW_WIDTH, WINDOW_HEIGHT);

    mfloat_t containerPos[VEC2_SIZE] = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};

    instantiateParticles(particles, NUM_PARTICLES);
    int activeParticles = 0;
    float spawnTimer = 0.0;

    float dt = 0.000001f;
    float lastFrameTime = (float)glfwGetTime();
    char title[100] = "";
    srand(time(NULL));

    float* instanceData = (float*)malloc(NUM_PARTICLES * 4 * sizeof(float));
    if (!instanceData) {
        fprintf(stderr, "Failed to allocate memory for instance data\n");
        glfwTerminate();
        return -1;
    }

    bool colorByVelocity = true;
    bool vKeyPressed = false; // To prevent toggling multiple times per key press

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Handle input to toggle color mode
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            if (!vKeyPressed) {
                colorByVelocity = !colorByVelocity;
                vKeyPressed = true;
            }
        } else {
            vKeyPressed = false;
        }

        spawnTimer += dt;
        if (1.0 / dt >= TARGET_FPS - 0.1 && spawnTimer >= SPAWN_DELAY && activeParticles < NUM_PARTICLES) {
            activeParticles += 1;
            spawnTimer = 0.0;
        }

        sprintf(title, "FPS : %-4.0f | Particles : %-10d", 1.0 / dt, activeParticles);
        glfwSetWindowTitle(window, title);

        // Update physics with multiple substeps for stability
        float sub_dt = dt / SUBSTEPS;
        for (int i = 0; i < SUBSTEPS; i++) {
            applyGravity(activeParticles);
            applyContainerConstraints(activeParticles, containerPos, CONTAINER);
            detectCollisions(activeParticles);
            updateParticlePositions(activeParticles, sub_dt);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Prepare instance data (positions and velocities)
        for (int i = 0; i < activeParticles; i++) {
            // Positions
            instanceData[4 * i] = particles[i].curr_position[0];
            instanceData[4 * i + 1] = particles[i].curr_position[1];

            // Velocities
            float vx = (particles[i].curr_position[0] - particles[i].old_position[0]) / dt;
            float vy = (particles[i].curr_position[1] - particles[i].old_position[1]) / dt;
            instanceData[4 * i + 2] = vx;
            instanceData[4 * i + 3] = vy;
        }

        // Draw container first
        draw_container(containerPos, CONTAINER);

        // Then draw particles
        draw_particles(activeParticles, instanceData, colorByVelocity);

        glfwSwapBuffers(window);
        glfwPollEvents();

        dt = (float)glfwGetTime() - lastFrameTime;
        while (dt < 1.0f / TARGET_FPS) {
            dt = (float)glfwGetTime() - lastFrameTime;
        }
        lastFrameTime = (float)glfwGetTime();
    }

    free(instanceData);
    cleanup_renderer();

    glfwTerminate();
    return 0;
}
