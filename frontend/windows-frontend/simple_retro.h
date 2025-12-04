#ifndef __SIMPLE_RETREO_H__
#define __SIMPLE_RETREO_H__
#include <string>
#include <vector>
#include <stdint.h>

typedef void (*simple_retro_video_refresh_t)(void*, const void *data, unsigned width, unsigned height, size_t pitch);
typedef void   (*simple_retro_input_poll_t)(void* device);
typedef int16_t (*simple_retro_input_state_t)(void*, unsigned port, unsigned device, unsigned index, unsigned id);

// singleton
class SimpleRetro
{
public:
    SimpleRetro(int argc, char**);
    ~SimpleRetro() = default;
public: // set callbacks
    void setVideoDeivce(void* device) { _video_deivce = device; }
    void setVideoRefresh(simple_retro_video_refresh_t cb);

    void setInputDeivce(void* device) { _input_deivce = device; }
    void setInputPoll(simple_retro_input_poll_t cb);
    void setInputState(simple_retro_input_state_t cb);

public: // basics
    void run();
    void reset();
    bool fail() const { return !_instance; }
private:
    static void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch);
    static void input_poll(void);
    static int16_t input_state(unsigned port, unsigned device, unsigned index, unsigned id);
private:
    simple_retro_video_refresh_t _video_refresh;
    simple_retro_input_poll_t    _input_poll;
    simple_retro_input_state_t   _input_state;
    
    std::vector<char> _rom_buffer;
    std::string _game_path;
    void *_video_deivce = nullptr;
    void *_input_deivce = nullptr;
    static SimpleRetro* _instance;
};


#define RETRO_DEVICE_ID_JOYPAD_B        0
#define RETRO_DEVICE_ID_JOYPAD_Y        1
#define RETRO_DEVICE_ID_JOYPAD_SELECT   2
#define RETRO_DEVICE_ID_JOYPAD_START    3
#define RETRO_DEVICE_ID_JOYPAD_UP       4
#define RETRO_DEVICE_ID_JOYPAD_DOWN     5
#define RETRO_DEVICE_ID_JOYPAD_LEFT     6
#define RETRO_DEVICE_ID_JOYPAD_RIGHT    7
#define RETRO_DEVICE_ID_JOYPAD_A        8
#define RETRO_DEVICE_ID_JOYPAD_X        9
#define RETRO_DEVICE_ID_JOYPAD_L       10
#define RETRO_DEVICE_ID_JOYPAD_R       11
#define RETRO_DEVICE_ID_JOYPAD_L2      12
#define RETRO_DEVICE_ID_JOYPAD_R2      13
#define RETRO_DEVICE_ID_JOYPAD_L3      14
#define RETRO_DEVICE_ID_JOYPAD_R3      15
#endif