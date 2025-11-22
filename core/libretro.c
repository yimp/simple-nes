#include "libretro.h"
#include "cpu.h"
#include "cart.h"
#include "ppu.h"
#include "dma.h"

static retro_video_refresh_t __video_cb;

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
    // r,g,b
    // 5(0-31), 6(0-63), 5(0-31)

    u16* buffer = NULL;
    while (!(buffer = ppu_frame_buffer()))
    {
        if (dma_processing()) {
            dma_clock();
        } else {
            cpu_clock();
        }
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

