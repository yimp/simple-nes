set(SRC
    "./libretro.c"
    "./cpu.c"
    "./bus.c"
    "./cart.c"
    "./ppu.c"
    "./dma.c"
    "./apu.c"
)

add_library(simple_nes_core STATIC ${SRC})