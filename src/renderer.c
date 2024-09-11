#include "renderer.h"
#include "physics.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846

// Number of segments to approximate a circle
#define CIRCLE_SEGMENTS 32

// Shader sources
const char* vertexShaderSource = 
"#version 330 core\n"
"layout(location = 0) in vec2 aVertexPosition;\n"
"layout(location = 1) in vec2 aInstancePosition;\n"
"uniform float uRadius;\n"
"uniform mat4 uProjection;\n"
"void main()\n"
"{\n"
"    vec2 position = aInstancePosition + aVertexPosition * uRadius;\n"
"    gl_Position = uProjection * vec4(position, 0.0, 1.0);\n"
"}\n";

const char* fragmentShaderSource = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 uColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(uColor, 1.0);\n"
"}\n";

// OpenGL object IDs
static GLuint shaderProgram;
static GLuint circleVBO;
static GLuint instanceVBO;
static GLuint VAO;

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check compile status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        const char* shaderType = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        fprintf(stderr, "ERROR::SHADER::%s::COMPILATION_FAILED\n%s\n", shaderType, infoLog);
    }

    return shader;
}

static GLuint link_program(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check link status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    // Shaders can be deleted after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


// Function to compute orthographic projection matrix
static void ortho(float left, float right, float bottom, float top, float near, float far, float* matrix) {
    // Column-major order
    matrix[0] = 2.0f / (right - left);
    matrix[1] = 0.0f;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;

    matrix[4] = 0.0f;
    matrix[5] = 2.0f / (top - bottom);
    matrix[6] = 0.0f;
    matrix[7] = 0.0f;

    matrix[8] = 0.0f;
    matrix[9] = 0.0f;
    matrix[10] = -2.0f / (far - near);
    matrix[11] = 0.0f;

    matrix[12] = -(right + left) / (right - left);
    matrix[13] = -(top + bottom) / (top - bottom);
    matrix[14] = -(far + near) / (far - near);
    matrix[15] = 1.0f;
}


void init_renderer(int window_width, int window_height) {
    // Compile and link shaders
    GLuint vertexShader = compile_shader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compile_shader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    shaderProgram = link_program(vertexShader, fragmentShader);

    // Generate and bind VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Generate circle geometry
    int numVertices = CIRCLE_SEGMENTS + 2; // +1 for closing the fan
    GLfloat* circleVertices = (GLfloat*)malloc(numVertices * 2 * sizeof(GLfloat));
    if (!circleVertices) {
        fprintf(stderr, "Failed to allocate memory for circle vertices\n");
        exit(-1);
    }

    // Center vertex
    circleVertices[0] = 0.0f;
    circleVertices[1] = 0.0f;

    // Perimeter vertices
    for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
        float angle = 2.0f * PI * ((float)i / CIRCLE_SEGMENTS);
        circleVertices[2 * (i + 1)] = cosf(angle);
        circleVertices[2 * (i + 1) + 1] = sinf(angle);
    }

    // Create and bind circle VBO
    glGenBuffers(1, &circleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, numVertices * 2 * sizeof(GLfloat), circleVertices, GL_STATIC_DRAW);
    free(circleVertices); // Data is now on GPU

    // Set vertex attribute for circle vertex positions (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);

    // Create and bind instance VBO
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    // Allocate buffer with maximum number of particles
    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 2 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

    // Set vertex attribute for instance positions (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glVertexAttribDivisor(1, 1); // Update per instance

    // Unbind VAO
    glBindVertexArray(0);

    // Compute and set the projection matrix
    float projection[16];
    ortho(0.0f, (float)window_width, 0.0f, (float)window_height, -1.0f, 1.0f, projection);

    // Use shader program to set projection uniform, radius, and color
    glUseProgram(shaderProgram);

    // Set projection matrix
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    // Set radius uniform (constant)
    GLint radiusLoc = glGetUniformLocation(shaderProgram, "uRadius");
    glUniform1f(radiusLoc, PARTICLE_RADIUS);

    // Set color uniform to light blue for particles
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    glUniform3f(colorLoc, 0.678f, 0.847f, 0.902f);

    glUseProgram(0);
}



void draw_container(mfloat_t* containerPos, int containerType) {
    glUseProgram(shaderProgram);

    // Set the color to white for the container
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);  // White color

    glLineWidth(CONTAINER_BORDER_WIDTH);

    if (containerType == 0) {  // Box
        float halfSize = CONTAINER_SIZE / 2.0f;
        GLfloat boxVertices[] = {
            containerPos[0] - halfSize, containerPos[1] - halfSize,
            containerPos[0] + halfSize, containerPos[1] - halfSize,
            containerPos[0] + halfSize, containerPos[1] + halfSize,
            containerPos[0] - halfSize, containerPos[1] + halfSize
        };

        GLuint boxVBO;
        glGenBuffers(1, &boxVBO);
        glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);

        glDrawArrays(GL_LINE_LOOP, 0, 4);

        glDisableVertexAttribArray(0);
        glDeleteBuffers(1, &boxVBO);
    } else if (containerType == 1) {  // Circle
        int numSegments = 100;
        GLfloat* circleVertices = (GLfloat*)malloc(numSegments * 2 * sizeof(GLfloat));
        if (!circleVertices) {
            fprintf(stderr, "Failed to allocate memory for container circle vertices\n");
            return;
        }

        for (int i = 0; i < numSegments; ++i) {
            float angle = 2.0f * PI * i / numSegments;
            circleVertices[2 * i] = containerPos[0] + cosf(angle) * CONTAINER_SIZE;
            circleVertices[2 * i + 1] = containerPos[1] + sinf(angle) * CONTAINER_SIZE;
        }

        GLuint circleVBO;
        glGenBuffers(1, &circleVBO);
        glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
        glBufferData(GL_ARRAY_BUFFER, numSegments * 2 * sizeof(GLfloat), circleVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);

        glDrawArrays(GL_LINE_LOOP, 0, numSegments);

        glDisableVertexAttribArray(0);
        glDeleteBuffers(1, &circleVBO);
        free(circleVertices);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}


void draw_particles(int activeParticles, float* positions) {
    glUseProgram(shaderProgram);

    float projection[16];
    ortho(0.0f, (float)WINDOW_WIDTH, 0.0f, (float)WINDOW_HEIGHT, -1.0f, 1.0f, projection);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, projection);

    // Set color to light blue for particles
    glUniform3f(glGetUniformLocation(shaderProgram, "uColor"), 0.5f, 0.8f, 1.f);

    glBindVertexArray(VAO);

    // Update instance VBO with positions
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, activeParticles * 2 * sizeof(float), positions);

    // Draw instanced particles
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS + 2, activeParticles);

    glBindVertexArray(0);
    glUseProgram(0);
}

void update_projection(int window_width, int window_height) {
    float projection[16];
    ortho(0.0f, (float)window_width, 0.0f, (float)window_height, -1.0f, 1.0f, projection);
    glUseProgram(shaderProgram);
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    glUseProgram(0);
}


void cleanup_renderer() {
    glDeleteBuffers(1, &circleVBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);
}



