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

    void startRendering() {
        // Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO &io = ImGui::GetIO();

        ImGui::ShowDemoWindow();

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
                                           | ImGuiWindowFlags_NoCollapse
                                           | ImGuiWindowFlags_NoSavedSettings;

        // const ImGuiViewport *viewport = ImGui::GetMainViewport();
        // ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(ImVec2(0, 0));
        ImGui::Begin("test", nullptr, flags);
    }

    void finishRendering() {
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

#endif //GUI_H
