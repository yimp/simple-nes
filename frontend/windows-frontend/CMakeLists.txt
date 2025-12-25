
set(SRC
    "./main.cc"
    "./logging.cc"
    "./simple_retro.cc"
    "./audio_sync.cc"
)
add_executable(hello ${SRC})
target_link_libraries(hello
    glfw
    opengl32
    miniaudio
    ${CMAKE_DL_LIBS}
)
add_dependencies(hello simple_nes_core)
add_custom_command(
    TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ARGS "${CMAKE_CURRENT_BINARY_DIR}/../../core/libsimple_nes_core.dll" "${CMAKE_CURRENT_BINARY_DIR}/libsimple_nes_core.dll"
)
