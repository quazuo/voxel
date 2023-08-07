#ifndef MYGE_RENDERER_H
#define MYGE_RENDERER_H

#include "GL/glew.h"

#include <string>
#include <vector>
#include <cstring>
#include <map>
#include <filesystem>

#include "src/voxel/chunk/chunk.h"
#include "src/utils/vec.h"

struct PackedVertex {
    Vec3 position;
    Vec2 uv;
    Vec3 normal;

    bool operator<(const PackedVertex that) const {
        return memcmp((void *) this, (void *) &that, sizeof(PackedVertex)) > 0;
    };
};

struct IndexedMeshData {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    // the indices are currently kept as unsigned shorts. this leads to a problem, because in an edge case
    // a chunk with `CHUNK_SIZE == 16` may contain more than USHRT_MAX rendered vertices.
    //
    // I'm currently ignoring these edge cases, as transparent blocks aren't implemented yet, and honestly
    // I can't come up with a pattern that would make this buffer overflow. hopefully, the problem will just
    // disappear in the future when I come up with some more elaborate way to exclude unseen blocks from rendering.
    std::vector<unsigned short> indices;
};

class MeshContext {
    std::vector<std::tuple<PackedVertex, PackedVertex, PackedVertex>> triangles{};

public:
    void addTriangle(PackedVertex &vertex1, PackedVertex &vertex2, PackedVertex &vertex3);

    [[nodiscard]]
    IndexedMeshData makeIndexed() const;

private:
    static void indexVertex(const PackedVertex &vertex, IndexedMeshData &data,
                            std::map<PackedVertex, unsigned short> &vertexToOutIndex);
};

class OpenGLRenderer {
    struct GLFWwindow *window = nullptr;

    bool isInit = false;

    Vec3 cameraPos{};
    Vec2 cameraRot{};

    GLuint vertexArrayID{};
    GLuint programID{};
    GLint mvpMatrixID{}, modelMatrixID{}, viewMatrixID{}, projectionMatrixID{};
    GLuint textureID{}, textureSamplerID{};
    GLint lightID{};

public:
    void init();

    void tick(float deltaTime);

    void startRendering();

    void finishRendering();

    [[nodiscard]]
    inline GLFWwindow *getWindow() const { return window; }

    void renderMesh(const MeshContext &ctx);

private:
    void loadShaders(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

    // todo - move this to a separate texture map class or smth like that
    void loadTextureDDS(const std::filesystem::path &path);

    void tickUserInputs(float deltaTime);
};

#endif //MYGE_RENDERER_H
