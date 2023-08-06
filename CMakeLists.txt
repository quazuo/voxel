cmake_minimum_required(VERSION 3.16)
project(voxel)

find_package(OpenGL REQUIRED)

# Compile external dependencies
add_subdirectory(deps)

include_directories(
    deps/glfw/include/
    deps/glm/
    deps/glew/include/
    .
)

set(ALL_LIBS
    ${OPENGL_LIBRARY}
    glfw
    GLEW_1130
)

add_definitions(
    -DTW_STATIC
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wpedantic -Wunused -Wcast-align \
    -Wdouble-promotion -Wmissing-declarations -Wmissing-include-dirs        \
    -Wnon-virtual-dtor -Wredundant-decls -Wodr -Wunreachable-code -Wshadow")

file(GLOB voxel_SRC
    "src/*"
    "src/utils/*"
    "src/render/*"
    "src/voxel/*"
    "src/voxel/chunk/*"
)
add_executable(voxel ${voxel_SRC})

target_link_libraries(voxel ${ALL_LIBS})
