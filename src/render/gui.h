#ifndef GUI_H
#define GUI_H

#include "deps/imgui/imgui.h"
#include "deps/imgui/backends/imgui_impl_glfw.h"
#include "deps/imgui/backends/imgui_impl_opengl3.h"

class GuiRenderer {
    GLFWwindow* window;

public:
    explicit GuiRenderer(GLFWwindow* w);

    ~GuiRenderer();
};

#endif //GUI_H
