#ifndef MYGE_RENDERER_H
#define MYGE_RENDERER_H

#include "GL/glew.h"

#include <vector>
#include <filesystem>

#include "src/voxel/chunk/chunk.h"
#include "texture-manager.h"
#include "camera.h"
#include "mesh-context.h"
#include "gl/gl-shader.h"
#include "gl/gl-vao.h"

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

    std::unique_ptr<GLShader> cubeShader, skyboxShader, lineShader, depthShader, debugDepthShader{}; // todo - remove!

    std::unique_ptr<ChunksVertexArray> chunksVao;

    struct Skybox {
        std::unique_ptr<BasicVertexArray> vao;
        glm::vec3 lightDirection = { 0.2, 0.3, 0.4 };
    } skybox;

    std::unique_ptr<BasicVertexArray> outlinesVao;

    std::unordered_map<LineType, glm::vec3> vertexGroupColors = {
            {CHUNK_OUTLINE,          {1, 1, 0}},
            {EMPTY_CHUNK_OUTLINE,    {1, 0, 0}},
            {SELECTED_BLOCK_OUTLINE, {0, 1, 1}},
    };
    std::unordered_map<LineType, std::vector<glm::vec3>> tempLineVertexGroups;

    std::unique_ptr<GLFrameBuffer> depthMap;
    static constexpr glm::ivec2 depthMapSize = {4096, 4096};

    struct {
        bool doDrawShadows = false;
        float frustumRadius = 160.f, nearPlane = 1.f, farPlane = 1000.f;
        float lightDistance = 200.f;
    } shadowConfig;
    float biasMin = 0.0005, biasFactor = 0.0007;

    // cached view and projection matrices and their product for the current render tick.
    // model matrix can't be cached because it's different for each chunk
    glm::mat4 viewMatrix{}, projectionMatrix{}, vpMatrix{}, lightVpMatrix{};

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
    const TextureManager& getTextureManager() const { return *textureManager; }

    [[nodiscard]]
    glm::vec3 getCameraPos() const { return camera->getPos(); }

    [[nodiscard]]
    std::vector<glm::ivec3> getLookedAtBlocks() const { return camera->getLookedAtBlocks(); }

    [[nodiscard]]
    bool isChunkInFrustum(const Chunk& chunk) const { return camera->isChunkInFrustum(chunk.getPos()); }

    [[nodiscard]]
    bool shouldDrawShadows() const { return shadowConfig.doDrawShadows; }

    /**
     * Locks or unlocks the cursor. When the cursor is locked, it's confined to the center
     * of the screen and camera rotates according to its movement. When it's unlocked, it's
     * visible and free to move around the screen; most importantly able to use the GUI.
     */
    void setIsCursorLocked(bool b) const;

    void writeChunkMesh(const Chunk::ChunkID id, const IndexedMeshData& mesh) const { chunksVao->writeChunk(id, mesh); }

    void freeChunkMesh(const Chunk::ChunkID id) const { chunksVao->eraseChunk(id); }

    void renderGuiSection();

    void renderSkybox() const;

    void startRenderingShadowMap();

    void makeChunkShadowMap(ChunkMeshContext &ctx) const;

    void finishRenderingShadowMap() const;

    void renderChunks(const std::vector<Chunk::ChunkID>& targets) const;

    /**
     * Adds a given chunk's outline to the list of lines that will be rendered later.
     *
     * @param chunkPos The chunk's position, given by its vertex with the lowest coordinates.
     * @param gid Outline type that should be used.
     */
    void addChunkOutline(const glm::ivec3 &chunkPos, LineType gid);

    /**
     * Adds a given block's outline to the list of lines that will be rendered later.
     *
     * @param blockPos The block's position, given by its vertex with the lowest coordinates.
     */
    void addTargetedBlockOutline(const glm::ivec3 &blockPos);

    /**
     * Renders the HUD, containing mostly text.
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
