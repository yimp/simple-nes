set(SRC
    "./libretro.c"
    "./cpu.c"
)

add_library(simple_nes_core STATIC ${SRC})