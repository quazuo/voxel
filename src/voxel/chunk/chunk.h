#ifndef MYGE_CHUNK_H
#define MYGE_CHUNK_H

#include <array>
#include <memory>
#include "src/voxel/block.h"
#include "src/utils/vec.h"

class Chunk {
public:
    Chunk() = default;

    explicit Chunk(Vec3 p) : pos(p) {}

    static constexpr int CHUNK_SIZE = 16;

    void load();

    void unload();

    [[nodiscard]]
    Vec3 getPos() const { return pos; }

    [[nodiscard]]
    bool isLoaded() const { return _isLoaded; }

    void render(class OpenGLRenderer &renderer);

    void updateBlock(int x, int y, int z, EBlockType type);

    void markDirty() { _isDirty = true; }

private:
    Vec3 pos;

    std::shared_ptr<class MeshContext> meshContext;
    bool isMesh = false;

    bool _isLoaded = false;
    bool _isDirty = true;

    std::array<std::array<std::array<Block, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE> blocks;

    void createMesh();

    void createCube(int x, int y, int z);

    void createFace(Vec3 v1, Vec3 v2, Vec3 v3, Vec3 v4, Vec2 uvOffset, Vec3 normal, EBlockType blockType);
};

#endif //MYGE_CHUNK_H
