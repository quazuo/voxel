#ifndef GL_VAO_H
#define GL_VAO_H

#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "gl-buffer.h"
#include "src/utils/size.h"
#include "src/voxel/chunk/chunk.h"

/**
 * Abstraction over an OpenGL Vertex Array Object. This is mainly used to hide API calls
 * to OpenGL for easier management and centralized implementation.
 */
class GLVertexArray {
protected:
    GLuint objectID {};

public:
    GLVertexArray();

    virtual ~GLVertexArray();

    /**
     * Enables the VAO. Used while rendering to inform OpenGL that we're using this VAO.
     */
    virtual void enable();
};

/**
 * Specialization of the GLVertexArray class, used for the purpose of holding all the data
 * relevant for meshes of all chunks, held in one VAO.
 *
 * This VAO wrapping class manages its own memory by means of a modified buddy algorithm.
 * The VAO's memory is divided into slabs of size `MAX_SECTOR_SIZE`. Whenever a block of memory
 * is requested inside the VAO, we first check if any currently used slab can accomodate enough
 * memory for the request. If any currently used slab can fit it (according to the buddy algorithm),
 * we allocate a fraction of that slab (called a sector) for this memory. If no currently used
 * slab can fulfill the request, we allocate a new slab for this purpose.
 */
class ChunksVertexArray final : public GLVertexArray {
    std::unique_ptr<GLArrayBuffer<glm::ivec3>> posBuffer;
    std::unique_ptr<GLArrayBuffer<glm::uint32>> normalUvTexBuffer;
    std::unique_ptr<GLElementBuffer> indicesBuffer;

    // min number of indices in a non-empty chunk is 3, because there are 3 indices in just one triangle.
    // realistically this is actually 36, because a minimal non-empty chunk has 1 block which has 6 faces,
    // thus 36 indices.
    static constexpr size_t MIN_SECTOR_SIZE = 3;

    // max number of indices in a chunk:
    // `CHUNK_SIZE^3 / 2` (every other block filled) x `2 * N_FACES` (each face has 2 tris) x 3 (indices per tri).
    static constexpr size_t MAX_CHUNK_INDEX_COUNT = 3 * SizeUtils::pow(Chunk::CHUNK_SIZE, 3) * EBlockFace::N_FACES;

    // number of different possible sector sizes
    static constexpr size_t SECTOR_LVL_COUNT = SizeUtils::log(2, MAX_CHUNK_INDEX_COUNT / MIN_SECTOR_SIZE) + 1;

    static constexpr size_t MAX_SECTOR_SIZE = 3 * SizeUtils::pow(2, SECTOR_LVL_COUNT - 1);

    static constexpr size_t SLAB_SIZE = MAX_SECTOR_SIZE;

    using SlabID = unsigned int;
    using SectorLevel = unsigned int;

    /**
     * A slab is a piece of contiguous memory of fixed size (`SLAB_SIZE`), fragmented into sectors.
     * Sectors' possible sizes are power-of-two fractions of the slab's size. As such, there is
     * a limited amount of possible sizes for sectors, signified by `SECTOR_LVL_COUNT`.
     */
    struct SlabData {
        /*
         * `levels[i]` holds information about which sectors of size `MIN_SECTOR_SIZE * 2^i`
         * are currently free.
         */
        std::array<std::set<off_t>, SECTOR_LVL_COUNT> levels {};

        SlabData() { levels[SECTOR_LVL_COUNT - 1].emplace(0); }
    };

    /**
     * A sector is a contiguous piece of memory inside a slab.
     */
    struct SectorData {
        SlabID slabID;
        off_t offset;
        SectorLevel level;
        size_t size;
    };

    /**
     * Helper structure that holds information about all slabs the system is currently aware of.
     * It primarily keeps track of slabs which are currently in some part allocated, as well as
     * which slabs should be created whenever one would be needed.
     */
    struct SlabsState {
        std::unordered_map<SlabID, SlabData> usedSlabs;
        std::priority_queue<SlabID, std::vector<SlabID>, std::greater<>> freedSlabs;
        SlabID nextFreshSlab = 0;

        [[nodiscard]]
        SlabID requestNewSlab();

        [[nodiscard]]
        SectorData requestNewSector(SectorLevel level);

        void reclaimSector(const SectorData& sector);

    private:
        [[nodiscard]]
        std::optional<SectorData> requestSectorFromSlab(SlabData& slabData, SlabID slabID, SectorLevel level) const;
    };

    /**
     * As all information about vertices (position, normal, UV, texID) is always grouped
     * in a way that their respective buffers always hold the same number of elements,
     * we allocate slabs and sectors for these pieces of data uniformly. Indices, however,
     * differ in this regard so all allocation regarding the indices buffer is performed
     * separately.
     */
    SlabsState vertexSlabsState, indexSlabsState;

    /**
     * Helper structure for remembering which sectors currently hold data relevant for
     * a given chunk.
     */
    struct ChunkSectorsData {
        SectorData vertexSector, indexSector;
    };

    std::unordered_map<Chunk::ChunkID, ChunkSectorsData> chunkSectorMapping;

public:
    ChunksVertexArray();

    void writeChunk(Chunk::ChunkID chunkID, const struct IndexedMeshData& mesh);

    void eraseChunk(Chunk::ChunkID chunkID);

    void render(const std::vector<Chunk::ChunkID>& targets) const;

private:
    [[nodiscard]]
    static SectorLevel calcSectorLevel(size_t dataSize);

    [[nodiscard]]
    static size_t calcSectorSize(SectorLevel level);
};

/**
 * Basic specialization of the GLVertexArray class for simple things which don't require
 * anything apart from a list of vertices to be rendered, like outlines or the skybox.
 */
class BasicVertexArray final : public GLVertexArray {
    std::unique_ptr<GLArrayBuffer<glm::vec3>> vertices;

public:
    BasicVertexArray();

    void writeToBuffers(const std::vector<glm::vec3>& data) const;
};

#endif //GL_VAO_H
