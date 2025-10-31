#include "libretro.h"
#include "cpu.h"
#include "cart.h"

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

    cpu_clock();

    static short buffer[256*240];
    for (int i = 0; i < 256*240; i++) {
        buffer[i] = cpu_x_y();
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

