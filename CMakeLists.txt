cmake_minimum_required(VERSION 3.16)
project(voxel)

find_package(OpenGL REQUIRED)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

find_library(libnoise NAMES "libnoise" "libnoise0" "noise" PATHS "lib")

# Compile external dependencies
add_subdirectory(deps)

include_directories(
        deps/glfw/include/
        deps/glm/
        deps/glew/include/
        include/
        .
)

set(ALL_LIBS
        ${OPENGL_LIBRARY}
        glfw
        GLEW_1130
        ${libnoise}
)

add_definitions(
        -DTW_STATIC
        -DTW_NO_LIB_PRAGMA
        -DTW_NO_DIRECT3D
        -DGLEW_STATIC
        -D_CRT_SECURE_NO_WARNINGS
)

#set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wpedantic -Wcast-align          \
#    -Wdouble-promotion -Wmissing-declarations -Wmissing-include-dirs        \
#    -Wnon-virtual-dtor -Wredundant-decls -Wodr -Wunreachable-code -Wshadow")

file(GLOB IMGUI_SRCS
        "deps/imgui/*.h"
        "deps/imgui/*.cpp"
)

set(IMGUI_IMPL_SRCS
        deps/imgui/backends/imgui_impl_glfw.h
        deps/imgui/backends/imgui_impl_glfw.cpp
        deps/imgui/backends/imgui_impl_opengl3.h
        deps/imgui/backends/imgui_impl_opengl3.cpp
)

set_source_files_properties(
        ${IMGUI_SRCS}
        ${IMGUI_IMPL_SRCS}
        PROPERTIES COMPILE_FLAGS "-w"
)

file(GLOB voxel_SRCS
        "src/*"
        "src/utils/*"
        "src/render/*"
        "src/render/gl/*"
        "src/voxel/*"
        "src/voxel/block/*"
        "src/voxel/chunk/*"
        "deps/noiseutils/*"
        "deps/stb/*"
)
add_executable(voxel ${voxel_SRCS} ${IMGUI_SRCS} ${IMGUI_IMPL_SRCS})

target_link_libraries(voxel ${ALL_LIBS})
