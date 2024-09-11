// app.c
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "renderer.h"
#include "physics.h"
#include <time.h>
#include <string.h>

#define PARTICLE_SPAWN_X (WINDOW_WIDTH / 4)
#define PARTICLE_SPAWN_Y (WINDOW_HEIGHT)

#define TARGET_FPS 60.0
#define SPAWN_DELAY 0.05

#define SUBSTEPS 8

#define CONTAINER 0 // box = 0, circle = 1

int elapsedFrames = 0;

Particle* particles = NULL;  // Declare as a pointer

// Function to instantiate particles (no changes needed here)
void instantiateParticles(int numParticles) {
    particles = (Particle*)malloc(NUM_PARTICLES * sizeof(Particle));  // Allocate memory dynamically
    for (int i = 0; i < numParticles; i++) {
        Particle* p = &(particles[i]);

        // ====== DEFAULT ======
        // mfloat_t x = PARTICLE_SPAWN_X + 300;
        // mfloat_t y = PARTICLE_SPAWN_Y - 100;
        // vec2(p->curr_position, x, y);
        // vec2(p->old_position, x, y);
        // vec2(p->acceleration, 0, 0);
        // p->radius = PARTICLE_RADIUS;
        // p->next = NULL;        

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
        // p->next = NULL;

        // ===== STREAM =====
        int distance = 2.0f;
        mfloat_t x = PARTICLE_SPAWN_X + ((i) % distance - distance / 2);
        mfloat_t y = PARTICLE_SPAWN_Y;
        mfloat_t xp = x * 1.01;
        mfloat_t yp = y * 0.998;
        vec2(p->curr_position, x, y);
        vec2(p->old_position, xp, yp);
        vec2(p->acceleration, 0, 0);
        p->radius = PARTICLE_RADIUS;
        p->next = NULL;
    }
}

void cleanupParticles() {
    free(particles);  // Clean up allocated memory
}

// Forward declaration of projection update
void update_projection(int window_width, int window_height);

// Callback to handle window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window; // Explicitly ignore the window parameter
    glViewport(0, 0, width, height);
    update_projection(width, height);
}

int main(void) {
    GLFWwindow* window;

    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Set GLFW window hints if necessary (e.g., OpenGL version)
    glfwWindowHint(GLFW_SAMPLES, 4); // anti aliasing

    // Create GLFW window
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Verlet Physics Engine", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Make the OpenGL context current
    glfwMakeContextCurrent(window);

    // Set framebuffer size callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Enable V-Sync
    glfwSwapInterval(1);

    // Initialize GLEW after making context current
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW: %s\n", glewGetErrorString(err));
        glfwTerminate();
        return -1;
    }

    // Optional: get version info
    /*
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);
    */

    // OpenGL settings
    glClearColor(0.15, 0.15, 0.15, 1.0); // window background color
    glClearStencil(0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(2.0);

    // Initialize the renderer for instanced rendering
    init_renderer(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // initialize the grid
    initGrid();

    // Initialize container position
    mfloat_t containerPos[VEC2_SIZE] = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};

    // Instantiate all particles (initial setup)
    instantiateParticles(NUM_PARTICLES);
    int activeParticles = 0;
    float spawnTimer = 0.0;

    // Timing variables
    float dt = 0.000001f;
    float lastFrameTime = (float)glfwGetTime();
    char title[100] = "";
    srand(time(NULL));

    // Allocate memory for instance data (positions)
    // Each particle has 2 floats: x and y
    float* instanceData = (float*)malloc(NUM_PARTICLES * 2 * sizeof(float));
    if (!instanceData) {
        fprintf(stderr, "Failed to allocate memory for instance data\n");
        glfwTerminate();
        return -1;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Handle input: close window on ESC
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Update spawn timer and activate particles based on target FPS and spawn delay
        spawnTimer += dt;
        if (1.0 / dt >= TARGET_FPS - 0.01 && spawnTimer >= SPAWN_DELAY && activeParticles < NUM_PARTICLES) {
            activeParticles += 1;
            spawnTimer = 0.0;
        }

        // Update window title with FPS and active particle count
        sprintf(title, "FPS : %-4.0f | Particles : %-10d", 1.0 / dt, activeParticles);
        glfwSetWindowTitle(window, title);

        // Update physics with multiple substeps for stability
        float sub_dt = dt / SUBSTEPS;
        for (int i = 0; i < SUBSTEPS; i++) {
            applyGravity(activeParticles);
            
            detectCollisions(activeParticles);

            applyContainerConstraints(activeParticles, containerPos, CONTAINER);

            updateParticlePositions(activeParticles, sub_dt);
        }

        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Prepare instance data (positions of active particles)
        for (int i = 0; i < activeParticles; i++) {
            instanceData[2 * i] = particles[i].curr_position[0];
            instanceData[2 * i + 1] = particles[i].curr_position[1];
        }

        // Draw particles using instanced rendering
        draw_particles(activeParticles, instanceData);

        // Draw container
        draw_container(containerPos, CONTAINER);
        
        // Swap buffers and poll for events
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Calculate delta time for the next frame
        dt = (float)glfwGetTime() - lastFrameTime;
        while (dt < 1.0f / TARGET_FPS) {
            dt = (float)glfwGetTime() - lastFrameTime;
        }
        lastFrameTime = (float)glfwGetTime();
    }

    // Clean up allocated memory and renderer resources
    free(instanceData);
    cleanup_renderer();
    cleanupGrid();
    cleanupParticles();
    // Terminate GLFW
    glfwTerminate();
    return 0;
}
