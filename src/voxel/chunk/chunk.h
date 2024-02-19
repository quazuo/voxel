#ifndef MYGE_CHUNK_H
#define MYGE_CHUNK_H

#include <array>
#include "src/voxel/block/block.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "src/utils/vec.h"
#include "src/utils/cube-array.h"

/**
 * A chunk groups up nearby blocks into cubes of `CHUNK_SIZE` width.
 * It's used primarily as an optimization tool, as managing singular blocks is very ineffective.
 */
class Chunk {
public:
    using ChunkID = unsigned int;

    static constexpr int CHUNK_SIZE = 16;

private:
    ChunkID id;

    // coordinates of the block with the lowest coordinates
    glm::ivec3 pos{};

    bool isMesh = false;

    bool _isLoaded = false;
    bool _isDirty = true;

    CubeArray<Block, CHUNK_SIZE> blocks;
    size_t activeBlockCount = 0;

public:
    explicit Chunk(const ChunkID i, const glm::ivec3 &p) : id(i), pos(p) {}

    [[nodiscard]]
    ChunkID getID() const { return id; }

    [[nodiscard]]
    glm::ivec3 getPos() const { return pos; }

    [[nodiscard]]
    EBlockType getBlock(const int x, const int y, const int z) const { return blocks[x][y][z].blockType; }

    [[nodiscard]]
    EBlockType getBlock(const glm::ivec3 &v) const { return blocks[v.x][v.y][v.z].blockType; }

    [[nodiscard]]
    bool isLoaded() const { return _isLoaded; }

    [[nodiscard]]
    bool shouldRender() const;

    [[nodiscard]]
    bool isDirty() const { return _isDirty; }

    void markDirty() { _isDirty = true; }

    void updateBlock(const glm::ivec3 &block, EBlockType type);

    void updateBlock(int x, int y, int z, EBlockType type);

    /**
     * Uses a provided world generation module to generate the contents of this chunk.
     */
    void generate(class WorldGen &worldGen);

    /**
     * Unloads this chunk from memory, letting the ChunkManager free a ChunkSlot in which this chunk resides.
     */
    void unload();

    /**
     * Creates a new mesh for this chunk and writes the data to the given mesh context.
     * This does nothing if there weren't any changes to this chunk since it was last loaded.
     *
     * @param meshContext Mesh context to which data should be written.
     * @param textureManager Reference to the texture manager, so that we can deduce which texture each block uses.
     */
    void createMesh(class ChunkMeshContext& meshContext, const class TextureManager &textureManager);

private:
    /**
     * Adds a specific cube at coordinates [x, y, z] relative to the chunk's `pos` coordinates.
     */
    void createCube(int x, int y, int z, ChunkMeshContext& meshContext, const TextureManager &textureManager);

    /**
     * Adds a specific cube's face to this mesh.
     */
    void createFace(const glm::ivec3 &cubePos, EBlockFace face, EBlockType blockType,
                    ChunkMeshContext& meshContext, const TextureManager &textureManager) const;
};

#endif //MYGE_CHUNK_H
