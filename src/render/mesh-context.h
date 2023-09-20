#ifndef VOXEL_MESH_CONTEXT_H
#define VOXEL_MESH_CONTEXT_H

#include <vector>
#include <cstring>
#include <map>

#include "GL/glew.h"
#include "src/voxel/chunk/chunk.h"
#include "gl-buffer.h"

struct PackedVertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    int texSamplerID;

    bool operator<(const PackedVertex other) const {
        return memcmp((void *) this, (void *) &other, sizeof(PackedVertex)) > 0;
    };
};

struct IndexedMeshData {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<int> texIDs;

    // the indices are currently kept as unsigned shorts. this leads to a problem, because in an edge case
    // a chunk with `CHUNK_SIZE == 16` may contain more than USHRT_MAX rendered vertices.
    //
    // I'm currently ignoring these edge cases, as transparent blocks aren't implemented yet, and due to the
    // fact that I can't come up with a pattern that would make this buffer overflow, I just assume there isn't one.
    // hopefully, the problem will just disappear in the future when I come up with some more elaborate ways to
    // exclude unseen blocks from rendering or implement face merging.
    std::vector<unsigned short> indices;
};

class MeshContext {
    using Quad = std::pair<PackedVertex, PackedVertex>;

    std::vector<Quad> quads{};
    std::vector<std::tuple<PackedVertex, PackedVertex, PackedVertex>> triangles{};

    IndexedMeshData indexedData;

    bool isIndexed = false;

    GLArrayBuffer<glm::vec3> vertices, normals;
    GLArrayBuffer<glm::vec2> uvs;
    GLArrayBuffer<int> texIDs;
    GLElementBuffer indices;

public:
    glm::vec3 modelTranslate;

    bool isFreshlyUpdated = false;

    void clear();

    void addQuad(PackedVertex &min, PackedVertex &max);

    void addTriangle(PackedVertex &vertex1, PackedVertex &vertex2, PackedVertex &vertex3);

    void triangulateQuads();

    void mergeQuads();

    void makeIndexed();

    void initBuffers();

    void writeToBuffers();

    void freeBuffers();

    void drawElements();

private:
    static void indexVertex(const PackedVertex &vertex, IndexedMeshData &data,
                            std::map<PackedVertex, unsigned short> &vertexToOutIndex);

    static std::vector<Quad> mergeQuadMap(CubeArray<short, Chunk::CHUNK_SIZE> &quadMap, glm::vec3 normal);
};

#endif //VOXEL_MESH_CONTEXT_H
