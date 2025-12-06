#include "simple_retro.h"
#include "libretro.h"
#include "logging.h"
#include <fstream>

SimpleRetro* SimpleRetro::_instance;

// *.exe <rom.nes> 
SimpleRetro::SimpleRetro(int argc, char** argv)
{
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

void SimpleRetro::setInputPoll(simple_retro_input_poll_t cb)
{
    _input_poll = cb;
    retro_set_input_poll(input_poll);
}

void SimpleRetro::setInputState(simple_retro_input_state_t cb)
{
    _input_state = cb;
    retro_set_input_state(input_state);
}

void SimpleRetro::setAudioSampleBatch(simple_retro_audio_sample_batch_t cb)
{
    _audio_sample_batch = cb;
    retro_set_audio_sample_batch(audio_sample_batch);
}

void SimpleRetro::video_refresh(const void *data, unsigned width, unsigned height, size_t pitch)
{
    _instance->_video_refresh(_instance->_video_device, data, width, height, pitch);
}

void SimpleRetro::input_poll(void)
{
    _instance->_input_poll(_instance->_input_device);
}

int16_t SimpleRetro::input_state(unsigned port, unsigned device, unsigned index, unsigned id)
{
    return _instance->_input_state(_instance->_input_device, port, device, index, id);
}

size_t SimpleRetro::audio_sample_batch(const int16_t *data, size_t frames)
{
    return _instance->_audio_sample_batch(_instance->_audio_device, data, frames);
}
