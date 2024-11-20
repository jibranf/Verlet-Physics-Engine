// renderer.c
#include "renderer.h"
#include "physics.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846

#define GL_CHECK(stmt) do { \
    stmt; \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        fprintf(stderr, "OpenGL error %08x, at %s:%i - for %s\n", err, __FILE__, __LINE__, #stmt); \
        exit(1); \
    } \
} while (0)

// Shader sources for particles
const char* particleVertexShaderSource = 
"#version 330 core\n"
"layout(location = 0) in vec2 aPosition;\n"
"layout(location = 1) in vec2 aVelocity;\n"
"uniform float uRadius;\n"
"uniform mat4 uProjection;\n"
"out vec2 vVelocity;\n"
"void main()\n"
"{\n"
"    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);\n"
"    gl_PointSize = uRadius * 2.0;\n"
"    vVelocity = aVelocity;\n"
"}\n";

const char* particleFragmentShaderSource = 
"#version 330 core\n"
"in vec2 vVelocity;\n"
"out vec4 FragColor;\n"
"uniform vec3 uColor;\n"
"uniform bool uColorMode;\n"
"void main()\n"
"{\n"
"    vec2 coord = gl_PointCoord - vec2(0.5);\n"
"    if (length(coord) > 2.0)\n"
"        discard;\n"
"    if (uColorMode)\n"
"    {\n"
"        float speed = length(vVelocity);\n"
"        float maxSpeed = 100.0;\n"
"        float intensity = clamp(speed / maxSpeed, 0.0, 1.0);\n"
"        vec3 velocityColor = mix(vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 1.0), intensity);\n"
"        FragColor = vec4(velocityColor, 1.0);\n"
"    }\n"
"    else\n"
"    {\n"
"        FragColor = vec4(uColor, 1.0);\n"
"    }\n"
"}\n";

// Shader sources for container
const char* containerVertexShaderSource = 
"#version 330 core\n"
"layout(location = 0) in vec2 aPosition;\n"
"uniform mat4 uProjection;\n"
"void main()\n"
"{\n"
"    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);\n"
"}\n";

const char* containerFragmentShaderSource = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 uColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(uColor, 1.0);\n"
"}\n";

// OpenGL object IDs
static GLuint particleShaderProgram;
static GLuint containerShaderProgram;
static GLuint particleVBO;
static GLuint particleVAO;

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    // Check compile status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint infoLogLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* infoLog = (char*)malloc(infoLogLength);
        glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog);
        const char* shaderType = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        fprintf(stderr, "ERROR::SHADER::%s::COMPILATION_FAILED\n%s\n", shaderType, infoLog);
        free(infoLog);
        glDeleteShader(shader);
        return 0;
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
        GLint infoLogLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        char* infoLog = (char*)malloc(infoLogLength);
        glGetProgramInfoLog(program, infoLogLength, NULL, infoLog);
        fprintf(stderr, "ERROR::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        free(infoLog);
        glDeleteProgram(program);
        return 0;
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
    // Compile and link particle shaders
    GLuint particleVertexShader = compile_shader(GL_VERTEX_SHADER, particleVertexShaderSource);
    GLuint particleFragmentShader = compile_shader(GL_FRAGMENT_SHADER, particleFragmentShaderSource);
    particleShaderProgram = link_program(particleVertexShader, particleFragmentShader);

    // Compile and link container shaders
    GLuint containerVertexShader = compile_shader(GL_VERTEX_SHADER, containerVertexShaderSource);
    GLuint containerFragmentShader = compile_shader(GL_FRAGMENT_SHADER, containerFragmentShaderSource);
    containerShaderProgram = link_program(containerVertexShader, containerFragmentShader);

    // Generate and bind VAO for particles
    glGenVertexArrays(1, &particleVAO);
    glBindVertexArray(particleVAO);

    // Create and bind particle VBO
    glGenBuffers(1, &particleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    // Allocate buffer with maximum number of particles (position + velocity)
    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 4 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

    // Set vertex attributes
    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);

    // Velocity attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    // Unbind VAO and VBO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Compute and set the projection matrix
    float projection[16];
    ortho(0.0f, (float)window_width, 0.0f, (float)window_height, -1.0f, 1.0f, projection);

    // Set uniforms for particle shader
    glUseProgram(particleShaderProgram);

    // Set projection matrix
    GLint projLoc = glGetUniformLocation(particleShaderProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    // Set radius uniform
    GLint radiusLoc = glGetUniformLocation(particleShaderProgram, "uRadius");
    glUniform1f(radiusLoc, PARTICLE_RADIUS);

    // Set default color
    GLint colorLoc = glGetUniformLocation(particleShaderProgram, "uColor");
    glUniform3f(colorLoc, 0.678f, 0.847f, 0.902f); // Light blue

    glUseProgram(0);

    // Set uniforms for container shader
    glUseProgram(containerShaderProgram);

    // Set projection matrix
    GLint containerProjLoc = glGetUniformLocation(containerShaderProgram, "uProjection");
    glUniformMatrix4fv(containerProjLoc, 1, GL_FALSE, projection);

    // Set container color
    GLint containerColorLoc = glGetUniformLocation(containerShaderProgram, "uColor");
    glUniform3f(containerColorLoc, 1.0f, 1.0f, 1.0f); // White

    glUseProgram(0);

    // Enable point size control from shader
    glEnable(GL_PROGRAM_POINT_SIZE);
}

void draw_container(mfloat_t* containerPos, int containerType) {
    glUseProgram(containerShaderProgram);

    glBindVertexArray(0); // Use default VAO for container

    if (containerType == 0) {  // Box
        float halfSize = CONTAINER_SIZE;
        float borderWidth = CONTAINER_BORDER_WIDTH;

        // Four border rectangles (left, right, bottom, top)
        GLfloat borderVertices[4][8] = {
            // Left border
            {
                containerPos[0] - halfSize, containerPos[1] - halfSize,
                containerPos[0] - halfSize + borderWidth, containerPos[1] - halfSize,
                containerPos[0] - halfSize + borderWidth, containerPos[1] + halfSize,
                containerPos[0] - halfSize, containerPos[1] + halfSize
            },
            // Right border
            {
                containerPos[0] + halfSize - borderWidth, containerPos[1] - halfSize,
                containerPos[0] + halfSize, containerPos[1] - halfSize,
                containerPos[0] + halfSize, containerPos[1] + halfSize,
                containerPos[0] + halfSize - borderWidth, containerPos[1] + halfSize
            },
            // Bottom border
            {
                containerPos[0] - halfSize + borderWidth, containerPos[1] - halfSize,
                containerPos[0] + halfSize - borderWidth, containerPos[1] - halfSize,
                containerPos[0] + halfSize - borderWidth, containerPos[1] - halfSize + borderWidth,
                containerPos[0] - halfSize + borderWidth, containerPos[1] - halfSize + borderWidth
            },
            // Top border
            {
                containerPos[0] - halfSize + borderWidth, containerPos[1] + halfSize - borderWidth,
                containerPos[0] + halfSize - borderWidth, containerPos[1] + halfSize - borderWidth,
                containerPos[0] + halfSize - borderWidth, containerPos[1] + halfSize,
                containerPos[0] - halfSize + borderWidth, containerPos[1] + halfSize
            }
        };

        GLuint borderVBO;
        glGenBuffers(1, &borderVBO);
        glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);

        for (int i = 0; i < 4; i++) {
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 8, borderVertices[i], GL_STATIC_DRAW);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }

        glDisableVertexAttribArray(0);
        glDeleteBuffers(1, &borderVBO);
    } else if (containerType == 1) {  // Circle
        int numSegments = 100;
        float outerRadius = CONTAINER_SIZE;
        float innerRadius = CONTAINER_SIZE - CONTAINER_BORDER_WIDTH;
        int totalVertices = numSegments * 2;

        GLfloat* ringVertices = (GLfloat*)malloc(totalVertices * 2 * sizeof(GLfloat));
        if (!ringVertices) {
            fprintf(stderr, "Failed to allocate memory for container circle vertices\n");
            return;
        }

        for (int i = 0; i < numSegments; ++i) {
            float angle = 2.0f * PI * i / numSegments;

            // Outer circle vertex
            ringVertices[4 * i] = containerPos[0] + cosf(angle) * outerRadius;
            ringVertices[4 * i + 1] = containerPos[1] + sinf(angle) * outerRadius;

            // Inner circle vertex
            ringVertices[4 * i + 2] = containerPos[0] + cosf(angle) * innerRadius;
            ringVertices[4 * i + 3] = containerPos[1] + sinf(angle) * innerRadius;
        }

        GLuint ringVBO;
        glGenBuffers(1, &ringVBO);
        glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
        glBufferData(GL_ARRAY_BUFFER, totalVertices * 2 * sizeof(GLfloat), ringVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, totalVertices);

        glDisableVertexAttribArray(0);
        glDeleteBuffers(1, &ringVBO);
        free(ringVertices);
    }

    glUseProgram(0);
}

void draw_particles(int activeParticles, float* data, bool colorByVelocity) {
    GL_CHECK(glUseProgram(particleShaderProgram));

    // Set uColorMode uniform
    glUniform1i(glGetUniformLocation(particleShaderProgram, "uColorMode"), colorByVelocity ? 1 : 0);

    // Set radius uniform (in case it was changed)
    glUniform1f(glGetUniformLocation(particleShaderProgram, "uRadius"), PARTICLE_RADIUS);

    // Update projection matrix if needed
    float projection[16];
    ortho(0.0f, (float)WINDOW_WIDTH, 0.0f, (float)WINDOW_HEIGHT, -1.0f, 1.0f, projection);
    glUniformMatrix4fv(glGetUniformLocation(particleShaderProgram, "uProjection"), 1, GL_FALSE, projection);

    // Set default color if not coloring by velocity
    if (!colorByVelocity) {
        glUniform3f(glGetUniformLocation(particleShaderProgram, "uColor"), 0.5f, 0.8f, 1.f); // Light blue
    }

    glBindVertexArray(particleVAO);

    // Update particle VBO with positions and velocities
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, activeParticles * 4 * sizeof(float), data);

    // Draw particles
    glDrawArrays(GL_POINTS, 0, activeParticles);

    glBindVertexArray(0);
    glUseProgram(0);
}

void update_projection(int window_width, int window_height) {
    float projection[16];
    ortho(0.0f, (float)window_width, 0.0f, (float)window_height, -1.0f, 1.0f, projection);

    // Update projection in particle shader
    glUseProgram(particleShaderProgram);
    GLint projLoc = glGetUniformLocation(particleShaderProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    glUseProgram(0);

    // Update projection in container shader
    glUseProgram(containerShaderProgram);
    GLint containerProjLoc = glGetUniformLocation(containerShaderProgram, "uProjection");
    glUniformMatrix4fv(containerProjLoc, 1, GL_FALSE, projection);
    glUseProgram(0);
}

void cleanup_renderer() {
    glDeleteBuffers(1, &particleVBO);
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteProgram(particleShaderProgram);
    glDeleteProgram(containerShaderProgram);
}