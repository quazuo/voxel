#ifndef MYGE_CHUNK_H
#define MYGE_CHUNK_H

#include <array>
#include <memory>
#include <utility>
#include "src/voxel/block/block.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "src/utils/vec.h"
#include "src/utils/cube-array.h"

class TextureManager;
/**
 * A chunk groups up nearby blocks into cubes of `CHUNK_SIZE` width.
 * It's used primarily as an optimization tool, as managing singular blocks is very ineffective.
 */
class Chunk {
public:
    static constexpr int CHUNK_SIZE = 16;

private:
    // coordinates of the block with the lowest coordinates
    glm::ivec3 pos{};

    std::shared_ptr<class ChunkMeshContext> meshContext;
    bool isMesh = false;

    bool _isLoaded = false;
    bool _isDirty = true;

    CubeArray<Block, CHUNK_SIZE> blocks;
    size_t activeBlockCount = 0;

public:
    explicit Chunk(const glm::ivec3 &p) : pos(p) {
    }

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

    void bindMeshContext(std::shared_ptr<class ChunkMeshContext> ctx) { meshContext = std::move(ctx); }

    void markDirty() { _isDirty = true; }

    void updateBlock(const glm::ivec3 &block, EBlockType type);

    void updateBlock(int x, int y, int z, EBlockType type);

    /**
     * Uses a provided world generation module to generate the contents of this chunk.
     */
    void generate(const std::shared_ptr<class WorldGen> &worldGen);

    /**
     * Unloads this chunk from memory, letting the ChunkManager free a ChunkSlot in which this chunk resides.
     */
    void unload();

    void render(const std::shared_ptr<class OpenGLRenderer> &renderer);

private:
    /**
     * Creates a new mesh for this chunk. This requires the mesh context to be bound beforehand.
     * This does nothing if there weren't any changes to this chunk since it was last loaded.
     *
     * @param textureManager Reference to the texture manager, so that we can deduce which texture each block uses.
     */
    void createMesh(const TextureManager &textureManager);

    /**
     * Adds a specific cube at coordinates [x, y, z] relative to the chunk's `pos` coordinates.
     */
    void createCube(int x, int y, int z, const TextureManager &textureManager);

    /**
     * Adds a specific cube's face to this mesh.
     */
    void createFace(const glm::ivec3 &cubePos, EBlockFace face, EBlockType blockType,
                    const TextureManager &textureManager) const;
};

#endif //MYGE_CHUNK_H
