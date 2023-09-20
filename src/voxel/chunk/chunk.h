#ifndef MYGE_CHUNK_H
#define MYGE_CHUNK_H

#include <array>
#include <memory>
#include "src/voxel/block.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "src/utils/size.h"
#include "src/utils/vec.h"
#include "src/utils/cube-array.h"

class Chunk {
public:
    Chunk() = default;

    explicit Chunk(glm::vec3 p) : pos(p) {}

    static constexpr int CHUNK_SIZE = 16;

    /*
    the below offsets signify a standardized specific ordering of vertices of a cube,
    used primarily in the mesh constructing algorithm and the face merging algorithm.

    the vertices are numbered as follows:
       6--------7
      /|       /|
     / |      / |
    3--------2  |
    |  |     |  |
    |  5-----|--4
    | /      | /
    |/       |/
    0--------1
    where the front face is the 0-1-2-3 one.
    */
    static constexpr std::array<glm::vec3, 8> vertexOffsets = {
        {
            Block::RENDER_SIZE * glm::vec3(-0.5, -0.5, 0.5),  // 0
            Block::RENDER_SIZE * glm::vec3(0.5, -0.5, 0.5),   // 1
            Block::RENDER_SIZE * glm::vec3(0.5, 0.5, 0.5),    // 2
            Block::RENDER_SIZE * glm::vec3(-0.5, 0.5, 0.5),   // 3
            Block::RENDER_SIZE * glm::vec3(0.5, -0.5, -0.5),  // 4
            Block::RENDER_SIZE * glm::vec3(-0.5, -0.5, -0.5), // 5
            Block::RENDER_SIZE * glm::vec3(-0.5, 0.5, -0.5),  // 6
            Block::RENDER_SIZE * glm::vec3(0.5, 0.5, -0.5)    // 7
        }
    };

    static std::pair<glm::vec3, glm::vec3> getFaceCorners(EBlockFace face) {
        switch (face) {
            case Front:
                return {vertexOffsets[0], vertexOffsets[2]};
            case Back:
                return {vertexOffsets[4], vertexOffsets[6]};
            case Right:
                return {vertexOffsets[1], vertexOffsets[7]};
            case Left:
                return {vertexOffsets[5], vertexOffsets[3]};
            case Top:
                return {vertexOffsets[3], vertexOffsets[7]};
            case Bottom:
                return {vertexOffsets[5], vertexOffsets[1]};
            case N_FACES:
                throw std::runtime_error("invalid face value in getFaceCorners()");
        }
        return {};
    }

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

    void updateBlock(VecUtils::Vec3Discrete block, EBlockType type);

    void updateBlock(int x, int y, int z, EBlockType type);

    void markDirty() { _isDirty = true; }

private:
    VecUtils::Vec3Discrete pos{};

    std::shared_ptr<class MeshContext> meshContext;
    bool isMesh = false;

    bool _isLoaded = false;
    bool _isDirty = true;

    CubeArray<Block, CHUNK_SIZE> blocks;
    size_t activeBlockCount = 0;

    void createMesh();

    void createCube(int x, int y, int z);

    void createFace(glm::vec3 cubePos, EBlockFace face, EBlockType blockType);
};

#endif //MYGE_CHUNK_H
