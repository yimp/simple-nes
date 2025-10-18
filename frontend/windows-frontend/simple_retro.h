#ifndef __SIMPLE_RETREO_H__
#define __SIMPLE_RETREO_H__
#include <string>
#include <vector>
struct GLFWwindow;

typedef void (*simple_retro_video_refresh_t)(GLFWwindow* glfw, const void *data, unsigned width, unsigned height, size_t pitch);

// singleton
class SimpleRetro
{
public:
    SimpleRetro(GLFWwindow* window, int argc, char**);
    ~SimpleRetro() = default;
public: // set callbacks
    void setVideoRefresh(simple_retro_video_refresh_t cb);
public: // basics
    void run();
    void reset();
    bool fail() const { return !_instance; }
private:
    static void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);
private:
    simple_retro_video_refresh_t _video_refresh;
    
    std::vector<char> _rom_buffer;
    std::string _game_path;
    GLFWwindow *_window = nullptr;
    static SimpleRetro* _instance;
};

#endif