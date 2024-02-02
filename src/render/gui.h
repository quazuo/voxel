#ifndef GUI_H
#define GUI_H

#define IMGUI_DEFINE_MATH_OPERATORS

#include "GLFW/glfw3.h"
#include "deps/imgui/imgui.h"
#include "deps/imgui/backends/imgui_impl_glfw.h"
#include "deps/imgui/backends/imgui_impl_opengl3.h"

class GuiRenderer {
    GLFWwindow *window;

public:
    explicit GuiRenderer(GLFWwindow *w);

    ~GuiRenderer();

    void startRendering();

    void finishRendering();
};

#endif //GUI_H
