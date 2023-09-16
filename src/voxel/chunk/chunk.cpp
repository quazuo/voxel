#include <iostream>
#include "chunk.h"

#include "src/render/renderer.h"
#include "src/render/mesh-context.h"
#include "src/voxel/world-gen.h"

void Chunk::load() {
    // todo - check if is stored on the disk and if it is, load it, otherwise generate terrain for it
    // todo - all of this should probably be done in the chunk manager. or at least the disk thing

    WorldGen::setChunkGenCtx(pos);
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                blocks[x][y][z].blockType = WorldGen::getBlockTypeAt(x, y, z);

                if (blocks[x][y][z].blockType != BlockType_None)
                    activeBlockCount++;
            }
        }
    }

    _isLoaded = true;
}

void Chunk::unload() {
    _isLoaded = false;
}

void Chunk::updateBlock(VecUtils::Vec3Discrete block, EBlockType type) {
    updateBlock(block.x, block.y, block.z, type);
}

void Chunk::updateBlock(int x, int y, int z, EBlockType type) {
    blocks[x][y][z].blockType = type;
    _isDirty = true;
}

bool Chunk::shouldRender() const {
    if (activeBlockCount == 0)
        return false;
    if (!_isLoaded)
        return false;
    return true;
}

void Chunk::render(const std::shared_ptr<OpenGLRenderer> &renderer) {
    if (activeBlockCount == 0)
        return;

    if (_isDirty) {
        createMesh();
        _isDirty = false;
    }

    if (isMesh) {
        renderer->renderChunk(meshContext);
    }
}

void Chunk::createMesh() {
    if (!meshContext) {
        throw std::runtime_error("tried to call createMesh() with no bound meshContext");
    }

    meshContext->clear();
    meshContext->modelTranslate = pos * CHUNK_SIZE;

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (blocks[x][y][z].isNone())
                    continue;

                createCube(x, y, z);
            }
        }
    }

    meshContext->makeIndexed();
    meshContext->isFreshlyUpdated = true;
    isMesh = true;
}

void Chunk::createCube(const int x, const int y, const int z) {
    /*

    the vertices are numbered as follows:
       7--------8
      /|       /|
     / |      / |
    4--------3  |
    |  |     |  |
    |  6-----|--5
    | /      | /
    |/       |/
    1--------2
    where the front face is the 1-2-3-4 face.

    */

    glm::vec3 cubeIndexPos = {(double) x, (double) y, (double) z};
    glm::vec3 cubePos = cubeIndexPos * (float) Block::RENDER_SIZE;
    const float offset = Block::RENDER_SIZE / 2;

    glm::vec3 p1(cubePos.x - offset, cubePos.y - offset, cubePos.z + offset);
    glm::vec3 p2(cubePos.x + offset, cubePos.y - offset, cubePos.z + offset);
    glm::vec3 p3(cubePos.x + offset, cubePos.y + offset, cubePos.z + offset);
    glm::vec3 p4(cubePos.x - offset, cubePos.y + offset, cubePos.z + offset);
    glm::vec3 p5(cubePos.x + offset, cubePos.y - offset, cubePos.z - offset);
    glm::vec3 p6(cubePos.x - offset, cubePos.y - offset, cubePos.z - offset);
    glm::vec3 p7(cubePos.x - offset, cubePos.y + offset, cubePos.z - offset);
    glm::vec3 p8(cubePos.x + offset, cubePos.y + offset, cubePos.z - offset);

    EBlockType blockType = blocks[x][y][z].blockType;

    if (z == CHUNK_SIZE - 1 || blocks[x][y][z + 1].isNone()) { // front
        createFace(p1, p2, p3, p4, {0.0, 0.0}, {0.0, 0.0, 1.0}, blockType);
    }

    if (z == 0 || blocks[x][y][z - 1].isNone()) { // back
        createFace(p5, p6, p7, p8, {1.0 / 4, 0}, {0.0, 0.0, -1.0}, blockType);
    }

    if (x == CHUNK_SIZE - 1 || blocks[x + 1][y][z].isNone()) { // right
        createFace(p2, p5, p8, p3, {2.0 / 4, 0}, {1.0, 0.0, 0.0}, blockType);
    }

    if (x == 0 || blocks[x - 1][y][z].isNone()) { // left
        createFace(p6, p1, p4, p7, {3.0 / 4, 0}, {-1.0, 0.0, 0.0}, blockType);
    }

    if (y == CHUNK_SIZE - 1 || blocks[x][y + 1][z].isNone()) { // top
        createFace(p4, p3, p8, p7, {0, 1.0 / 4}, {0.0, 1.0, 0.0}, blockType);
    }

    if (y == 0 || blocks[x][y - 1][z].isNone()) { // bottom
        createFace(p6, p5, p2, p1, {1.0 / 4, 1.0 / 4}, {0.0, -1.0, 0.0}, blockType);
    }
}

void Chunk::createFace(const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3, const glm::vec3 v4,
                       const glm::vec2 uvOffset, const glm::vec3 normal, const EBlockType blockType) {
    // UV coordinates of the front face, used as a reference point for other faces
    constexpr glm::vec2 uvs[4] = {
        {0,       1.0 / 4},
        {1.0 / 4, 1.0 / 4},
        {1.0 / 4, 0},
        {0,       0}
    };

    // this offset is just a way to squeeze in info about which texture the vertex uses without the need
    // to use more memory. refer to the fragment shader to see how this x coordinate is handled.
    //
    // the "-1" is here because actual blocks (i.e. other than `BlockType_None`) start at index 1
    const glm::vec2 uvTexOffset = {(double) blockType - 1, 0};

    PackedVertex pv1 = {v1, uvs[0] + uvOffset + uvTexOffset, normal};
    PackedVertex pv2 = {v2, uvs[1] + uvOffset + uvTexOffset, normal};
    PackedVertex pv3 = {v3, uvs[2] + uvOffset + uvTexOffset, normal};
    PackedVertex pv4 = {v4, uvs[3] + uvOffset + uvTexOffset, normal};
    meshContext->addTriangle(pv1, pv2, pv3);
    meshContext->addTriangle(pv1, pv3, pv4);
}
