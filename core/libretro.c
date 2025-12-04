#include "libretro.h"
#include "cpu.h"
#include "cart.h"
#include "ppu.h"
#include "dma.h"
#include "bus.h"

static retro_video_refresh_t __video_cb;
static retro_input_poll_t __input_poll_cb;
static retro_input_state_t __input_state_cb;

RETRO_API void retro_init(void)
{

}

RETRO_API bool retro_load_game(const struct retro_game_info *game)
{
    cart_load(game->data);
    return true;
}


// called every frame
RETRO_API void retro_run(void)
{
    __input_poll_cb();
    u8 p1 = 0x00;
    p1 |= __input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) ? 0x01 : 0x00;
    p1 |= __input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) ? 0x02 : 0x00;
    p1 |= __input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT) ? 0x04 : 0x00;
    p1 |= __input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START) ? 0x08 : 0x00;
    p1 |= __input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ? 0x10 : 0x00;
    p1 |= __input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ? 0x20 : 0x00;
    p1 |= __input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ? 0x40 : 0x00;
    p1 |= __input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ? 0x80 : 0x00;
    bus_update_input(p1, 0x00);

    u16* buffer = NULL;
    while (!(buffer = ppu_frame_buffer()))
    {
        if (dma_processing()) {
            dma_clock();
        } else {
            cpu_clock();
        }
        ppu_clock();
        ppu_clock();
        if (ppu_nmi_triggered()) {
            cpu_nmi();
        }
    }

    __video_cb(buffer, 256, 240, 256 * 2);
}

RETRO_API void retro_reset(void)
{
    cpu_reset();
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb)
{
    __video_cb = cb;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t cb)
{
    __input_poll_cb = cb;
}

RETRO_API void retro_set_input_state(retro_input_state_t cb)
{
    __input_state_cb = cb;
}