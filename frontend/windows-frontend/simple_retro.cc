#include "simple_retro.h"
#include "libretro.h"
#include "logging.h"
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef _WIN32
    HMODULE dll_handle_ = nullptr;
#else
    void* dll_handle_ = nullptr;
#endif

// Core function pointers (with RETRO_CALLCONV calling convention)
void (RETRO_CALLCONV *retro_init_)(void) = nullptr;
void (RETRO_CALLCONV *retro_deinit_)(void) = nullptr;
unsigned (RETRO_CALLCONV *retro_api_version_)(void) = nullptr;
void (RETRO_CALLCONV *retro_get_system_info_)(struct retro_system_info*) = nullptr;
void (RETRO_CALLCONV *retro_get_system_av_info_)(struct retro_system_av_info*) = nullptr;
void (RETRO_CALLCONV *retro_set_controller_port_device_)(unsigned, unsigned) = nullptr;
void (RETRO_CALLCONV *retro_reset_)(void) = nullptr;
void (RETRO_CALLCONV *retro_run_)(void) = nullptr;
size_t (RETRO_CALLCONV *retro_serialize_size_)(void) = nullptr;
bool (RETRO_CALLCONV *retro_serialize_)(void*, size_t) = nullptr;
bool (RETRO_CALLCONV *retro_unserialize_)(const void*, size_t) = nullptr;
void (RETRO_CALLCONV *retro_cheat_reset_)(void) = nullptr;
void (RETRO_CALLCONV *retro_cheat_set_)(unsigned, bool, const char*) = nullptr;
bool (RETRO_CALLCONV *retro_load_game_)(const struct retro_game_info*) = nullptr;
bool (RETRO_CALLCONV *retro_load_game_special_)(unsigned, const struct retro_game_info*, size_t) = nullptr;
void (RETRO_CALLCONV *retro_unload_game_)(void) = nullptr;

// Callback setters
void (RETRO_CALLCONV *retro_set_environment_)(retro_environment_t) = nullptr;
void (RETRO_CALLCONV *retro_set_video_refresh_)(retro_video_refresh_t) = nullptr;
void (RETRO_CALLCONV *retro_set_audio_sample_)(retro_audio_sample_t) = nullptr;
void (RETRO_CALLCONV *retro_set_audio_sample_batch_)(retro_audio_sample_batch_t) = nullptr;
void (RETRO_CALLCONV *retro_set_input_poll_)(retro_input_poll_t) = nullptr;
void (RETRO_CALLCONV *retro_set_input_state_)(retro_input_state_t) = nullptr;

SimpleRetro* SimpleRetro::_instance;

bool LoadFunctionPointers() {
#ifdef _WIN32
#define GET_SYMBOL(name) \
    { \
        FARPROC proc = GetProcAddress(dll_handle_, #name); \
        if (!proc) { \
            std::cerr << "Failed to load symbol: " << #name << " (Error: " << GetLastError() << ")" << std::endl; \
            return false; \
        } \
        *(void**)(&name##_) = (void*)proc; \
        std::cout << "Loaded symbol: " << #name << " at address: 0x" << std::hex << (uint64_t)proc << std::endl; \
    }
#else
#define GET_SYMBOL(name) \
    *(void**)(&name##_) = dlsym(dll_handle_, #name); \
    if (!name##_) { \
        std::cerr << "Failed to load symbol: " << #name << " - " << dlerror() << std::endl; \
        return false; \
    }
#endif

    GET_SYMBOL(retro_init);
    // GET_SYMBOL(retro_deinit);
    // GET_SYMBOL(retro_api_version);
    // GET_SYMBOL(retro_get_system_info);
    // GET_SYMBOL(retro_get_system_av_info);
    // GET_SYMBOL(retro_set_controller_port_device);
    GET_SYMBOL(retro_reset);
    GET_SYMBOL(retro_run);
    // GET_SYMBOL(retro_serialize_size);
    // GET_SYMBOL(retro_serialize);
    // GET_SYMBOL(retro_unserialize);
    // GET_SYMBOL(retro_cheat_reset);
    // GET_SYMBOL(retro_cheat_set);
    GET_SYMBOL(retro_load_game);
    // GET_SYMBOL(retro_unload_game);
    // GET_SYMBOL(retro_set_environment);
    GET_SYMBOL(retro_set_video_refresh);
    // GET_SYMBOL(retro_set_audio_sample);
    GET_SYMBOL(retro_set_audio_sample_batch);
    GET_SYMBOL(retro_set_input_poll);
    GET_SYMBOL(retro_set_input_state);

    return true;
}

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

    // libretro function loader
    {
#ifdef _MSC_VER
        const std::string lib_prefix = "";
#else
        const std::string lib_prefix = "lib";
#endif

#ifdef _WIN32
        const std::string lib_suffix = ".dll";
#elif defined(__linux__)
        const std::string lib_suffix = ".so";
#elif defined(__APPLE__)
        const std::string lib_suffix = ".dylib";
#elif defined(__unix__) || defined(__unix) // other UNIX like OS (eg. BSD)
        const std::string lib_suffix = ".so";
#endif

        const std::string core_path = lib_prefix + "simple_nes_core" + lib_suffix;
#ifdef _WIN32
        dll_handle_ = LoadLibraryA(core_path.c_str());
        if (!dll_handle_) {
            DWORD error = GetLastError();
            LOG_ERROR("Failed to load core: %s\n", core_path.c_str());
            LOG_ERROR("Error code: %x\n", error);
            // Try to get more detailed error info
            char error_msg[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, error_msg, sizeof(error_msg), NULL);
            LOG_ERROR( "Error message: %s\n", error_msg);
            return;
        }
        LOG_INFO("Successfully loaded DLL: %s\n", core_path.c_str());
#else
        dll_handle_ = dlopen(core_path.c_str(), RTLD_LAZY);
        if (!dll_handle_) {
            LOG_ERROR("Failed to load core: %s - %x\n", core_path.c_str(), dlerror());
        }
        return;
#endif

        if (!LoadFunctionPointers()) {
            LOG_ERROR("Failed to load function pointer from: %s\n", core_path.c_str());
            return;
        }
    }

    _game_path = argv[1];
    retro_init_();
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
        if (!retro_load_game_(&info)) {
            LOG_ERROR("Failed to load rom:%s", _game_path.c_str());
            return;
        }
    }
    _instance = this;
}

void SimpleRetro::run()
{
    retro_run_();
}

void SimpleRetro::reset()
{
    retro_reset_();
}

void SimpleRetro::setVideoRefresh(simple_retro_video_refresh_t cb)
{
    _video_refresh = cb;
    retro_set_video_refresh_(video_refresh);
}

void SimpleRetro::setInputPoll(simple_retro_input_poll_t cb)
{
    _input_poll = cb;
    retro_set_input_poll_(input_poll);
}

void SimpleRetro::setInputState(simple_retro_input_state_t cb)
{
    _input_state = cb;
    retro_set_input_state_(input_state);
}

void SimpleRetro::setAudioSampleBatch(simple_retro_audio_sample_batch_t cb)
{
    _audio_sample_batch = cb;
    retro_set_audio_sample_batch_(audio_sample_batch);
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
