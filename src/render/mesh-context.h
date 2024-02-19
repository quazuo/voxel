#ifndef VOXEL_MESH_CONTEXT_H
#define VOXEL_MESH_CONTEXT_H

#include <vector>
#include <cstring>
#include <map>
#include <memory>
#include <optional>

#include "gl/gl-buffer.h"
#include "glm/glm.hpp"
#include "src/voxel/chunk/chunk.h"

/**
 * Structure holding all data describing a vertex.
 */
struct Vertex {
    glm::ivec3 position;
    glm::ivec2 uv;
    glm::vec3 normal;
    int texSamplerID;

    bool operator<(const Vertex other) const {
        return memcmp(this, &other, sizeof(Vertex)) > 0;
    }
};

/**
 * Structure holding all data describing a mesh after indexing has been performed on it.
 */
struct IndexedMeshData {
    std::vector<glm::ivec3> vertices;
    std::vector<glm::ivec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<int> texIDs;
    std::vector<GLElementBuffer::ElemType> indices;

    [[nodiscard]]
    std::vector<glm::uint32> packNormalUvTex() const;
};

/**
 * Class holding information about the state of a chunk's mesh we're creating and/or rendering.
 */
class ChunkMeshContext {
    using Quad = std::pair<Vertex, Vertex>;
    using Triangle = std::tuple<Vertex, Vertex, Vertex>;

    std::vector<Quad> quads{};
    std::vector<Triangle> triangles{};
    std::optional<IndexedMeshData> indexedData;

public:
    glm::vec3 modelTranslate{};

    bool isFreshlyUpdated = false;

    /**
     * Clears all the quads, triangles and indexed vertices from this mesh, only leaving
     * allocated GLBuffers intact.
     */
    void clear();

    /**
     * Adds a new quad to this mesh, described by its minimal and maximal vertices.
     *
     * @param min Vertex with the lowest coordinates in the quad.
     * @param max Vertex with the highest coordinates in the quad.
     */
    void addQuad(const Vertex &min, const Vertex &max);

    /**
     * Adds a new triangle to this mesh, described by its vertices.
     */
    [[maybe_unused]]
    void addTriangle(const Vertex &vertex1, const Vertex &vertex2, const Vertex &vertex3);

    /**
     * Splits all quads in this mesh into triangles.
     */
    void triangulateQuads();

    /**
     * Merges quads that are adjacent and parallel to each other, are facing the same way
     * and use the same texture. This drastically optimizes the amount of memory we need to use
     * for each chunk's mesh.
     */
    void mergeQuads();

    /**
     * Indexes the triangles in this mesh, optimizing the memory usage of this mesh.
     */
    void makeIndexed();

    /**
     * Returns this mesh' indexed data. This throws a runtime_error if this mesh wasn't indexed beforehand.
     */
    [[nodiscard]]
    const IndexedMeshData& getIndexedData() const { return *indexedData; }

private:
    /**
     * Merges a set of quads facing the same direction, i.e. all up-facing quads or down-facing or etc.
     *
     * @param quadMap List of quads facing the same direction.
     * @param normal The normal vector shared by all the quads.
     * @return List of merged quads.
     */
    static std::vector<Quad> mergeQuadMap(CubeArray<short, Chunk::CHUNK_SIZE> &quadMap, const glm::vec3 &normal);
};

#endif //VOXEL_MESH_CONTEXT_H
