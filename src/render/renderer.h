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
    glm::vec2 windowSize;

    std::shared_ptr<class TextureManager> texManager = std::make_shared<TextureManager>();

    KeyManager keyManager;

    bool isInit = false;

    Camera camera{};

    // OpenGL handles for various objects
    GLuint vertexArrayID{};
    GLuint textVertexBufferID{}, textUVBufferID;
    GLuint cubeShaderID{}, lineShaderID{}, textShaderID{};
    GLuint lineVertexArrayID{};
    GLint mvpMatrixID{}, modelMatrixID{}, viewMatrixID{}, projectionMatrixID{};
    GLint lightID{};

    // cached view and projection matrices for the current render tick
    // model matrix can't be cached because it's different for each chunk
    glm::mat4 viewMatrix, projectionMatrix;

public:
    struct GLFWwindow *init(int windowWidth, int windowHeight);

    void tick(float deltaTime);

    void terminate();

    // should be called before any rendering
    void startRendering();

    // should be called after all rendering in the current tick has been finished
    void finishRendering();

    [[nodiscard]]
    inline GLFWwindow *getWindow() const { return window; }

    [[nodiscard]]
    glm::vec3 getCameraPos() const { return camera.pos; }

    [[nodiscard]]
    std::vector<VecUtils::Vec3Discrete> getLookedAtBlocks() const { return camera.getLookedAtBlocks(); }

    [[nodiscard]]
    bool isChunkInFrustum(const Chunk& chunk) const { return camera.isChunkInFrustum(chunk.getPos()); };

    void renderChunk(const std::shared_ptr<MeshContext>& ctx);

    void renderChunkOutline(glm::vec3 chunkPos, glm::vec3 color) const;

    void renderTargetedBlockOutline(glm::vec3 blockPos) const;

    void renderText(const std::string& text, int x, int y, size_t fontSize) const;

    void renderHud() const;

private:
    void renderCubeOutline(glm::vec3 minVec, float sideLength, glm::vec3 color) const;

    void renderOutline(const std::vector<glm::vec3> &vertices, const glm::mat4& mvpMatrix, glm::vec3 color) const;

    void tickMouseMovement(float deltaTime);

    GLuint loadShaders(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

    void loadTextures() const;

    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam);
};

#endif //MYGE_RENDERER_H
