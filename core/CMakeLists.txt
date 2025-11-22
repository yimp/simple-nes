set(SRC
    "./libretro.c"
    "./cpu.c"
    "./bus.c"
    "./cart.c"
    "./ppu.c"
    "./dma.c"
)

add_library(simple_nes_core STATIC ${SRC})