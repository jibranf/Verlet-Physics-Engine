#include "renderer.h"
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include "physics.h"

#define PI 3.14159265358979323846

static int window_width, window_height;

void init_window(int width, int height) {
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void draw_container(mfloat_t* containerPos, int container) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(CONTAINER_BORDER_WIDTH);
    if (container == 0) { // draw box 
        glBegin(GL_LINE_LOOP);
        glVertex2f(containerPos[0] - CONTAINER_SIZE, containerPos[1] - CONTAINER_SIZE);
        glVertex2f(containerPos[0] + CONTAINER_SIZE, containerPos[1] - CONTAINER_SIZE);
        glVertex2f(containerPos[0] + CONTAINER_SIZE, containerPos[1] + CONTAINER_SIZE);
        glVertex2f(containerPos[0] - CONTAINER_SIZE, containerPos[1] + CONTAINER_SIZE);
        glEnd();
    }
    if (container == 1) { // draw circle
        float theta = PI * 2 / 360.0;
        float tangential_factor = tanf(theta);
        float radial_factor = cosf(theta);
        float x = CONTAINER_SIZE; // radius
        float y = 0;
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 360; i++) {
            glVertex2f((window_width / 2) + x, (window_height / 2) + y);
            float tx = -y;
            float ty = x;
            x += tx * tangential_factor;
            y += ty * tangential_factor;
            x *= radial_factor;
            y *= radial_factor;
        }
        glEnd();
    }
}

void draw_particle(float x, float y, float radius) {
    glColor3f(0.5f, 0.8f, 1.0f);  // Light blue color
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 360; i++) {
        float radian = i * (PI / 180.0f);
        glVertex2f(x + cos(radian) * radius, y + sin(radian) * radius);
    }
    glEnd();
}
