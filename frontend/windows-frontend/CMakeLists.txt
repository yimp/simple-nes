
set(SRC
    "./main.cc"
    "./logging.cc"
    "./simple_retro.cc"
)
add_executable(hello ${SRC})
target_link_libraries(hello
    simple_nes_core
    glfw
    opengl32
)