set(SRC
    "./libretro.c"
    "./cpu.c"
    "./bus.c"
    "./cart.c"
    "./ppu.c"
    "./dma.c"
    "./apu.c"
    "./mapper.c"
    "./mappers/NROM.c"
    "./mappers/UxROM.c"
    "./mappers/MMC3.c"
    "./mappers/246.c"
)

add_library(simple_nes_core STATIC ${SRC})