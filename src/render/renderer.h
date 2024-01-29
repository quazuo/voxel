#ifndef MYGE_RENDERER_H
#define MYGE_RENDERER_H

#include "GL/glew.h"

#include <string>
#include <vector>
#include <map>
#include <filesystem>

#include "src/voxel/chunk/chunk.h"
#include "texture-manager.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/trigonometric.hpp"
#include "camera.h"
#include "gl-buffer.h"
#include "mesh-context.h"

/**
 * The main renderer of the program. There should only be one instance of this class, as it
 * doesn't really make sense to have two renderers.
 */
class OpenGLRenderer {
public:
    enum LineType {
        CHUNK_OUTLINE,
        EMPTY_CHUNK_OUTLINE,
        SELECTED_BLOCK_OUTLINE
    };

private:
    GLFWwindow *window;

    std::unique_ptr<TextureManager> textureManager = std::make_unique<TextureManager>();

    KeyManager keyManager;

    std::unique_ptr<Camera> camera;

    // OpenGL handles
    GLuint vertexArrayID{};
    GLuint cubeShaderID{}, lineShaderID{}, textShaderID{};
    GLint mvpMatrixID{}, modelMatrixID{}, viewMatrixID{}, projectionMatrixID{};
    GLint lightID{};

    // OpenGL buffers used for text rendering
    std::unique_ptr<GLArrayBuffer<glm::vec2>> textVertices, textUVs;

    // OpenGL buffers used for outline rendering
    std::unique_ptr<GLArrayBuffer<glm::vec3>> lineVertices;

    std::map<LineType, glm::vec3> vertexGroupColors = {
            {CHUNK_OUTLINE,          {1, 1, 0}},
            {EMPTY_CHUNK_OUTLINE,    {1, 0, 0}},
            {SELECTED_BLOCK_OUTLINE, {0, 1, 1}},
    };
    std::map<LineType, std::vector<glm::vec3>> tempLineVertexGroups;

    // cached view and projection matrices for the current render tick
    // model matrix can't be cached because it's different for each chunk
    glm::mat4 viewMatrix{}, projectionMatrix{};

public:
    OpenGLRenderer(int windowWidth, int windowHeight);

    ~OpenGLRenderer();

    void tick(float deltaTime) const;

    /**
     * Starts the rendering process.
     * Should be called before any rendering is done.
     */
    void startRendering();

    /**
     * Wraps up the rendering process.
     * Should be called after all rendering in the current tick has been finished.
     */
    void finishRendering() const;

    [[nodiscard]]
    GLFWwindow *getWindow() const { return window; }

    [[nodiscard]]
    glm::vec3 getCameraPos() const { return camera->getPos(); }

    [[nodiscard]]
    std::vector<VecUtils::Vec3Discrete> getLookedAtBlocks() const { return camera->getLookedAtBlocks(); }

    [[nodiscard]]
    bool isChunkInFrustum(const Chunk& chunk) const { return camera->isChunkInFrustum(chunk.getPos()); };

    void renderChunk(const std::shared_ptr<ChunkMeshContext>& ctx);

    /**
     * Adds a given chunk's outline to the list of lines that will be rendered later.
     *
     * @param chunkPos The chunk's position, given by its vertex with the lowest coordinates.
     * @param gid Outline type that should be used.
     */
    void addChunkOutline(const glm::vec3 &chunkPos, LineType gid);

    /**
     * Adds a given block's outline to the list of lines that will be rendered later.
     *
     * @param blockPos The block's position, given by its vertex with the lowest coordinates.
     */
    void addTargetedBlockOutline(const glm::vec3 &blockPos);

    /**
     * Renders a specified string of text at given coordinates.
     *
     * @param text The text to be rendered.
     * @param x X coordinates of the down-left corner of the text box.
     * @param y Y coordinates of the down-left corner of the text box.
     * @param fontSize The size of the text.
     */
    void renderText(const std::string& text, float x, float y, float fontSize) const;

    /**
     * Renders the HUD, containing mostly text.
     * TODO: Will be later mostly replaced by the addition of ImGUI.
     */
    void renderHud() const;

    /**
     * Renders all the outlines added by previously defined functions.
     */
    void renderOutlines();

private:
    /**
     * Adds a given axis-aligned cube's outline to the list of lines that will be rendered later.
     *
     * @param minVec Position of the cube's vertex with lowest coordinates.
     * @param sideLength The cube's side length.
     * @param gid The type of outline that should be used.
     */
    void addCubeOutline(const glm::vec3 &minVec, float sideLength, LineType gid);

    static GLuint loadShaders(const std::filesystem::path &vertexShaderPath,
                              const std::filesystem::path &fragmentShaderPath);

    void loadTextures() const;

    /**
     * Debug callback used by GLFW to notify the user of errors.
     */
    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar *message, const void *userParam);

    static void windowRefreshCallback(GLFWwindow *window);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
};

#endif //MYGE_RENDERER_H
