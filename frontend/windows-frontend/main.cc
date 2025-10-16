#include "logging.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glext.h>

void error_callback(int error_code, const char* description)
{
    LOG_ERROR("error:%d, msg:%s", error_code, description);
}

void render0()
{
    glClearColor(0.2f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(-0.6f, -0.4f);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f( 0.6f,  0.4f);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f( 0.0f,  0.6f);
    glEnd();
}

void render1()
{
    glClearColor(0.2f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();
}

#include <random>
void update_texture()
{
    static GLuint texture = -1;
    if (texture == -1)
    {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    static std::vector<short> random_data(256 * 240);
    {
        static std::default_random_engine engine;
        for (int i = 0; i < random_data.size(); i++)
        {
            random_data[i] = engine() & 1 ? 0x0000 : 0xFFFF;
        }
    }

    // r,g,b
    // 5(0-31), 6(0-63), 5(0-31)
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, random_data.data());
    glEnable(GL_TEXTURE_2D);
}

void render(int w, int h)
{
    const float nes_aspect = 256.0f / 240.0f;
    float aspect = (float)w / (float)h;

    float w_scale = 1.0f;
    float h_scale = 1.0f;
    if (aspect > nes_aspect)
    {
        w_scale = nes_aspect / aspect;
    } else {
        h_scale = aspect / nes_aspect;
    }

    glClearColor(0.2f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, w, h);

    update_texture();
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-w_scale, -h_scale);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( w_scale, -h_scale);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( w_scale,  h_scale);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-w_scale,  h_scale);
    glEnd();
}

int main(int argc, char** argv)
{
    LOG_TRACE("Open game rom:%s", "mario.nes", "", 1234);
    LOG_TRACE("rom size:%d", 123);
    LOG_DEBUG("debug:%d", 123);
    LOG_INFO("info:%d", 123);
    LOG_WARN("warn:%d", 123);
    LOG_ERROR("error:%d", 123);

    glfwInit();
    glfwSetErrorCallback(error_callback);
    GLFWwindow* window = glfwCreateWindow(256, 240, "simple-nes", nullptr, nullptr);
    glfwSwapInterval(1);
    glfwMakeContextCurrent(window);

    int w, h;
    while (!glfwWindowShouldClose(window))
    {
        // render
        glfwGetFramebufferSize(window, &w, &h);
        render(w, h);
        glfwSwapBuffers(window);

        // events
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            LOG_TRACE("A pressed");
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}