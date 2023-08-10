#ifndef MYGE_RENDERER_H
#define MYGE_RENDERER_H

#include "GL/glew.h"

#include <string>
#include <vector>
#include <cstring>
#include <map>
#include <filesystem>

#include "src/voxel/chunk/chunk.h"
#include "texture-manager.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/trigonometric.hpp"
#include "camera.h"

class OpenGLRenderer {
    struct GLFWwindow *window = nullptr;

    std::shared_ptr<class TextureManager> texManager = std::make_shared<TextureManager>();

    bool isInit = false;

    bool doRenderChunkOutlines = true;

    Camera camera{};

    // OpenGL handles for various objects
    GLuint vertexArrayID{};
    GLuint cubeShaderID{}, lineShaderID{};
    GLuint lineVertexArrayID{};
    GLint mvpMatrixID{}, modelMatrixID{}, viewMatrixID{}, projectionMatrixID{};
    GLint lightID{};

public:
    void init();

    void tick(float deltaTime);

    void startRendering();

    void finishRendering();

    [[nodiscard]]
    inline GLFWwindow *getWindow() const { return window; }

    void renderChunk(const MeshContext &ctx);

private:
    GLuint loadShaders(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

    void loadTextures() const;

    void tickUserInputs(float deltaTime);

    void renderLine(glm::vec3 start, glm::vec3 end, glm::mat4 mvpMatrix) const;

    void updateCameraFrustum();

    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam);
};

#endif //MYGE_RENDERER_H
