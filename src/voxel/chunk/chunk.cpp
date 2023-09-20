#include <iostream>
#include "chunk.h"

#include "src/render/renderer.h"
#include "src/render/mesh-context.h"
#include "src/voxel/world-gen.h"

void Chunk::load() {
    // todo - check if is stored on the disk and if it is, load it, otherwise generate terrain for it
    // todo - all of this should probably be done in the chunk manager. or at least the disk thing

    WorldGen::setChunkGenCtx(pos);
    blocks.forEach([&](int x, int y, int z, Block& b) {
        b.blockType = WorldGen::getBlockTypeAt(x, y, z);

        if (blocks[x][y][z].blockType != BlockType_None)
            activeBlockCount++;
    });

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

    blocks.forEach([&](int x, int y, int z, Block& b) {
        if (b.isNone()) return;
        createCube(x, y, z);
    });

    meshContext->mergeQuads();
    meshContext->triangulateQuads();
    meshContext->makeIndexed();
    meshContext->isFreshlyUpdated = true;
    isMesh = true;
}

void Chunk::createCube(const int x, const int y, const int z) {
    glm::vec3 cubeIndexPos = {x, y, z};
    glm::vec3 cubePos = cubeIndexPos * (float) Block::RENDER_SIZE;

    // these points are indexed as mentioned in the header file.
    constexpr size_t N_POINTS = 8;
    std::array<glm::vec3, N_POINTS> points = {};
    for (size_t i = 0; i < N_POINTS; i++) {
        points[i] = cubePos + vertexOffsets[i];
    }

    EBlockType blockType = blocks[x][y][z].blockType;

    if (z == CHUNK_SIZE - 1 || blocks[x][y][z + 1].isNone()) {
        createFace(cubePos, Front, blockType);
    }

    if (z == 0 || blocks[x][y][z - 1].isNone()) {
        createFace(cubePos, Back, blockType);
    }

    if (x == CHUNK_SIZE - 1 || blocks[x + 1][y][z].isNone()) {
        createFace(cubePos, Right, blockType);
    }

    if (x == 0 || blocks[x - 1][y][z].isNone()) {
        createFace(cubePos, Left, blockType);
    }

    if (y == CHUNK_SIZE - 1 || blocks[x][y + 1][z].isNone()) {
        createFace(cubePos, Top, blockType);
    }

    if (y == 0 || blocks[x][y - 1][z].isNone()) {
        createFace(cubePos, Bottom, blockType);
    }
}

void Chunk::createFace(const glm::vec3 cubePos, const EBlockFace face, const EBlockType blockType) {
    static const std::map<EBlockFace, glm::vec3> faceNormals = {
        {Front,  {0.0,  0.0,  1.0}},
        {Back,   {0.0,  0.0,  -1.0}},
        {Right,  {1.0,  0.0,  0.0}},
        {Left,   {-1.0, 0.0,  0.0}},
        {Top,    {0.0,  1.0,  0.0}},
        {Bottom, {0.0,  -1.0, 0.0}}
    };

    const auto corners = getFaceCorners(face);
    const glm::vec3 minPos = cubePos + corners.first;
    const glm::vec3 maxPos = cubePos + corners.second;

    PackedVertex packedMin = {minPos, {0, 1.0}, faceNormals.at(face),
                              TextureManager::getBlockSamplerID(blockType, face)};
    PackedVertex packedMax = {maxPos, {1.0, 0}, faceNormals.at(face),
                              TextureManager::getBlockSamplerID(blockType, face)};
    meshContext->addQuad(packedMin, packedMax);
}
