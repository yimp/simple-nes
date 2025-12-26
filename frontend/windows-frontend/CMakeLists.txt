
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
if (WIN32)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_custom_command(
            TARGET hello POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ARGS "${CMAKE_CURRENT_BINARY_DIR}/../../core/simple_nes_core.dll" "${CMAKE_CURRENT_BINARY_DIR}/simple_nes_core.dll"
        )
    else ()
        add_custom_command(
            TARGET hello POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ARGS "${CMAKE_CURRENT_BINARY_DIR}/../../core/libsimple_nes_core.dll" "${CMAKE_CURRENT_BINARY_DIR}/libsimple_nes_core.dll"
        )
    endif ()
elseif (APPLE)
    add_custom_command(
        TARGET hello POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ARGS "${CMAKE_CURRENT_BINARY_DIR}/../../core/libsimple_nes_core.dylib" "${CMAKE_CURRENT_BINARY_DIR}/libsimple_nes_core.dylib"
    )
elseif (UNIX AND NOT APPLE) # *NIX
    add_custom_command(
        TARGET hello POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ARGS "${CMAKE_CURRENT_BINARY_DIR}/../../core/libsimple_nes_core.so" "${CMAKE_CURRENT_BINARY_DIR}/libsimple_nes_core.so"
    )
endif ()