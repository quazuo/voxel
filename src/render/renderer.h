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
#include "gl-buffer.h"

class OpenGLRenderer {
public:
    enum LineVertexGroup {
        CHUNK,
        CHUNK_EMPTY,
        SELECTED_BLOCK
    };

private:
    struct GLFWwindow *window = nullptr;
    glm::vec2 windowSize;

    std::shared_ptr<class TextureManager> textureManager = std::make_shared<TextureManager>();

    KeyManager keyManager;

    std::shared_ptr<Camera> camera;

    // OpenGL handles for various objects
    GLuint vertexArrayID{};
    GLuint cubeShaderID{}, lineShaderID{}, textShaderID{};
    GLint mvpMatrixID{}, modelMatrixID{}, viewMatrixID{}, projectionMatrixID{};
    GLint lightID{};

    // OpenGL buffers used for text rendering
    GLArrayBuffer<glm::vec2> textVertices, textUVs;

    // OpenGL buffers used for outline rendering
    GLArrayBuffer<glm::vec3> lineVertices;

    std::map<LineVertexGroup, glm::vec3> vertexGroupColors = {
            { CHUNK, {1, 1, 0}},
            { CHUNK_EMPTY, {1, 0, 0}},
            { SELECTED_BLOCK, {0, 1, 1}},
    };
    std::map<LineVertexGroup, std::vector<glm::vec3>> tempLineVertexGroups;

    // cached view and projection matrices for the current render tick
    // model matrix can't be cached because it's different for each chunk
    glm::mat4 viewMatrix{}, projectionMatrix{};

public:
    OpenGLRenderer(int windowWidth, int windowHeight);

    void tick(float deltaTime);

    void terminate();

    /// starts the rendering process.
    /// should be called before any rendering is done
    void startRendering();

    /// wraps up the rendering process.
    /// should be called after all rendering in the current tick has been finished
    void finishRendering();

    [[nodiscard]]
    inline GLFWwindow *getWindow() const { return window; }

    [[nodiscard]]
    glm::vec3 getCameraPos() const { return camera->getPos(); }

    [[nodiscard]]
    std::vector<VecUtils::Vec3Discrete> getLookedAtBlocks() const { return camera->getLookedAtBlocks(); }

    [[nodiscard]]
    bool isChunkInFrustum(const Chunk& chunk) const { return camera->isChunkInFrustum(chunk.getPos()); };

    void renderChunk(const std::shared_ptr<MeshContext>& ctx);

    void addChunkOutline(glm::vec3 chunkPos, LineVertexGroup gid);

    void addTargetedBlockOutline(glm::vec3 blockPos);

    void renderText(const std::string& text, float x, float y, size_t fontSize);

    void renderHud();

    void renderOutlines();

private:
    void addCubeOutline(glm::vec3 minVec, float sideLength, LineVertexGroup gid);

    void tickMouseMovement(float deltaTime);

    static GLuint loadShaders(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

    void loadTextures() const;

    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam);
};

#endif //MYGE_RENDERER_H
