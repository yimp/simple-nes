#include "logging.h"
#include "simple_retro.h"
#include "audio_sync.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glext.h>

void error_callback(int error_code, const char* description)
{
    LOG_ERROR("error:%d, msg:%s", error_code, description);
}


#include <random>
void update_texture(const void *data, unsigned width, unsigned height, size_t pitch)
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

    // r,g,b
    // 5(0-31), 6(0-63), 5(0-31)
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 256);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, data);
    glEnable(GL_TEXTURE_2D);
}

void render(void* device, const void *data, unsigned width, unsigned height, size_t pitch)
{
    int w, h;
    GLFWwindow* glfw = (GLFWwindow*)(device);

    if (!glfwGetCurrentContext()) {
        glfwMakeContextCurrent(glfw);
        glfwSwapInterval(1);
    }

    glfwGetFramebufferSize(glfw, &w, &h);

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

    update_texture(data, width, height, pitch);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-w_scale, -h_scale);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( w_scale, -h_scale);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( w_scale,  h_scale);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-w_scale,  h_scale);
    glEnd();
    glfwSwapBuffers(glfw);
}

void    input_poll(void* device) { }
int16_t input_state(void* pdevice, unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (port != 0 || device != 1 || id >= 12)
        return 0;
    GLFWwindow* window = (GLFWwindow*)pdevice;
    switch (id)
    {
    case RETRO_DEVICE_ID_JOYPAD_A:      return glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_B:      return glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_SELECT: return glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_START:  return glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_L:      return glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_R:      return glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_UP:     return glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_DOWN:   return glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_LEFT:   return glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    case RETRO_DEVICE_ID_JOYPAD_RIGHT:  return glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    default:
        break;
    }
    return 0;
}

int main(int argc, char** argv)
{
    glfwInit();
    glfwSetErrorCallback(error_callback);
    GLFWwindow* window = glfwCreateWindow(256, 240, "simple-nes", nullptr, nullptr);

    SimpleRetro retro(argc, argv);
    retro.setVideoDevice(window);
    retro.setVideoRefresh(render);
    retro.setInputDevice(window);
    retro.setInputPoll(input_poll);
    retro.setInputState(input_state);
    retro.reset();

    auto audio = audio_sync::start(&retro);
    if (!audio) {
        goto EXIT;
    }

    while (!glfwWindowShouldClose(window))
    {
        // events
        glfwWaitEvents();
    }

EXIT:
    audio_sync::destroy(audio);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}