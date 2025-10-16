
set(SRC
    "./main.cc"
    "./logging.cc"
)
add_executable(hello ${SRC})
target_link_libraries(hello
    glfw
    opengl32
)