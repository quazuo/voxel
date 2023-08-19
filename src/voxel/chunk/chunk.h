#ifndef MYGE_CHUNK_H
#define MYGE_CHUNK_H

#include <array>
#include <memory>
#include "src/voxel/block.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "src/utils/size.h"
#include "src/utils/vec.h"

class Chunk {
public:
    Chunk() = default;

    explicit Chunk(glm::vec3 p) : pos(p) {}

    static constexpr int CHUNK_SIZE = 16;

    void load();

    void unload();

    void bindMeshContext(std::shared_ptr<class MeshContext> ctx) { meshContext = ctx; }

    [[nodiscard]]
    VecUtils::Vec3Discrete getPos() const { return pos; }

    [[nodiscard]]
    bool isLoaded() const { return _isLoaded; }

    [[nodiscard]]
    bool shouldRender() const;

    void render(const std::shared_ptr<class OpenGLRenderer> &renderer);

    [[nodiscard]]
    EBlockType getBlock(int x, int y, int z) const { return blocks[x][y][z].blockType; }

    [[nodiscard]]
    EBlockType getBlock(VecUtils::Vec3Discrete v) const { return blocks[v.x][v.y][v.z].blockType; }

    void updateBlock(int x, int y, int z, EBlockType type);

    void markDirty() { _isDirty = true; }

private:
    VecUtils::Vec3Discrete pos{};

    std::shared_ptr<class MeshContext> meshContext;
    bool isMesh = false;

    bool _isLoaded = false;
    bool _isDirty = true;

    SizeUtils::CubeArray<Block, CHUNK_SIZE> blocks;
    size_t activeBlockCount = 0;

    void createMesh();

    void createCube(int x, int y, int z);

    void createFace(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4,
                    glm::vec2 uvOffset, glm::vec3 normal, EBlockType blockType);
};

#endif //MYGE_CHUNK_H
