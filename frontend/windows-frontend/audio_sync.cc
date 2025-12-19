#include "audio_sync.h"
#include "logging.h"
#include <miniaudio.h>

#define SAMPLE_RATE (44100)
#define CHANELS 2
#define SAMPLES_PER_FRAME 735

namespace audio_sync
{
typedef int16_t sample_t;
const int bytes_per_sample = sizeof(sample_t) * CHANELS;
typedef struct
{
    ma_device device;
    ma_device_config device_config;

    char buffer[SAMPLES_PER_FRAME * bytes_per_sample];
    int buffer_beg = 0;
    int buffer_end = 0;

    SimpleRetro* retro;
} audio_context_t;

void device_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
size_t simple_retro_audio_sample_batch(void* device, const int16_t *data, size_t frames);

context_t start(SimpleRetro* retro)
{
    audio_context_t* context = new audio_context_t;
    context->device_config = ma_device_config_init(ma_device_type_playback);
    context->device_config.playback.channels = CHANELS;
    context->device_config.playback.format = ma_format_s16;
    context->device_config.sampleRate = SAMPLE_RATE;
    context->device_config.dataCallback = device_data_callback;

    ma_result ret = ma_device_init(nullptr, &context->device_config, &context->device);
    if (ret != MA_SUCCESS) {
        LOG_ERROR("Failed to init audio device, ret=%d", ret);
        delete context;
        return nullptr;
    }

    retro->setAudioDevice(context);
    retro->setAudioSampleBatch(simple_retro_audio_sample_batch);
    context->retro = retro;
    context->device.pUserData = context;
    ret = ma_device_start(&context->device);
    if (ret != MA_SUCCESS) {
        LOG_ERROR("Failed to start audio device, ret=%d", ret);
        delete context;
        return nullptr;
    }
    return context;
}


void destroy(context_t context)
{
    if (context) {
        audio_context_t* audio_context = (audio_context_t*)context;
        ma_device_stop(&audio_context->device);
        delete audio_context;
    }
}

void device_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    audio_context_t* context = (audio_context_t*)(pDevice->pUserData);
    char* stream = (char*)pOutput;

OUTPUT:
    if (context->buffer_end - context->buffer_beg >= frameCount) {
        memcpy(stream, context->buffer + context->buffer_beg * bytes_per_sample, frameCount * bytes_per_sample);
        context->buffer_beg += frameCount;
        stream += frameCount * bytes_per_sample;
        return;
    }

    while (frameCount) {
        if (context->buffer_end - context->buffer_beg > 0) {
            int bytes_to_copy = (context->buffer_end - context->buffer_beg) * bytes_per_sample;
            memcpy(stream, context->buffer + context->buffer_beg * bytes_per_sample, bytes_to_copy);
            frameCount -= (context->buffer_end - context->buffer_beg);
            stream += bytes_to_copy;
            context->buffer_beg = context->buffer_end = 0;
            continue;
        }

        {
            static int frame = 0; ;
            static Timestamp last_frame = Timestamp::now();
            const int x = 100;
            // fill audio buffer by performing a video frame
            context->retro->run();

            ++frame;
            if ((frame % x) == 0) {
                double elapse = (double)(Timestamp::now().us() - last_frame.us());
                elapse /= (double)x;
                LOG_TRACE("frame %d, fps=%.2f", frame, 1000000.0f / elapse);
                last_frame = Timestamp::now();
            }
            goto OUTPUT;
        }
    }
}

size_t simple_retro_audio_sample_batch(void* device, const int16_t *data, size_t frames)
{
    audio_context_t* audio_context = (audio_context_t*)device;
    if (frames > SAMPLES_PER_FRAME) {
        frames = SAMPLES_PER_FRAME;
    }
    memcpy(audio_context->buffer, data, frames * bytes_per_sample);
    audio_context->buffer_beg = 0;
    audio_context->buffer_end = frames;
    return frames;
}

} // namespace audio_sync