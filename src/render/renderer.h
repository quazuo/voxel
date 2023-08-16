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

    Camera camera{};

    // OpenGL handles for various objects
    GLuint vertexArrayID{};
    GLuint cubeShaderID{}, lineShaderID{};
    GLuint lineVertexArrayID{};
    GLint mvpMatrixID{}, modelMatrixID{}, viewMatrixID{}, projectionMatrixID{};
    GLint lightID{};

    // cached view and projection matrices for the current render tick
    // model matrix can't be cached because it's different for each chunk
    glm::mat4 viewMatrix, projectionMatrix;

public:
    void init();

    void tick(float deltaTime);

    // should be called before any rendering
    void startRendering();

    // should be called after all rendering in the current tick has been finished
    void finishRendering();

    [[nodiscard]]
    inline GLFWwindow *getWindow() const { return window; }

    [[nodiscard]]
    glm::vec3 getCameraPos() const { return camera.pos; }

    [[nodiscard]]
    bool isChunkInFrustum(const Chunk& chunk) const;

    void renderChunk(const std::shared_ptr<MeshContext>& ctx);

    void renderChunkOutline(glm::vec3 chunkPos, glm::vec3 color) const;

private:
    void renderOutline(const std::vector<glm::vec3> &vertices, const glm::mat4& mvpMatrix, glm::vec3 color) const;

    void renderFrustumOutline() const;

    GLuint loadShaders(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

    void loadTextures() const;

    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam);
};

#endif //MYGE_RENDERER_H
