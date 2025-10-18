#include "simple_retro.h"
#include "libretro.h"
#include "logging.h"
#include <fstream>

SimpleRetro* SimpleRetro::_instance;

// *.exe <rom.nes> 
SimpleRetro::SimpleRetro(GLFWwindow* window, int argc, char** argv)
{
    _window = window;

    if (_instance) {
        std::abort();
    }

    if (argc != 2) {
        LOG_ERROR("Invalid arguments: %d", argc);
        return;
    }

    _game_path = argv[1];
    retro_init();
    std::ifstream ifs(_game_path, std::ios::binary);
    if (!ifs.is_open()) {
        LOG_ERROR("Failed to open:%s,errno=%d", _game_path.c_str(), errno);
        return;
    }
    else {
        retro_game_info info;
        ifs.seekg(0, std::ios::end);
        info.size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        _rom_buffer.resize(info.size);
        ifs.read(_rom_buffer.data(), info.size);
        if (ifs.fail()) {
            LOG_ERROR("Failed to read:%s,errno=%d", _game_path.c_str(), errno);
            return;
        }

        info.data = _rom_buffer.data();
        if (!retro_load_game(&info)) {
            LOG_ERROR("Failed to load rom:%s", _game_path.c_str());
            return;
        }
    }
    _instance = this;
}

void SimpleRetro::run()
{
    retro_run();
}

void SimpleRetro::reset()
{
    retro_reset();
}

void SimpleRetro::setVideoRefresh(simple_retro_video_refresh_t cb)
{
    _video_refresh = cb;
    retro_set_video_refresh(video_refresh);
}


void SimpleRetro::video_refresh(const void *data, unsigned width, unsigned height, size_t pitch)
{
    _instance->_video_refresh(_instance->_window, data, width, height, pitch);
}
