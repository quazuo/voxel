#include "chunk.h"

#include <iostream>

#include "src/render/renderer.h"
#include "src/render/mesh-context.h"
#include "src/voxel/world-gen.h"

void Chunk::generate(const OpenGLRenderer &renderer, WorldGen &worldGen) {
    worldGen.fillChunk(pos, blocks);

    blocks.forEach([&](const int x, const int y, const int z, Block &b) {
        (void)(x + y + z); // warning silencer
        if (!b.isNone()) {
            activeBlockCount++;
        }
    });

    createMesh(renderer.getTextureManager());
    _isLoaded = true;
}

void Chunk::unload() {
    _isLoaded = false;
}

void Chunk::updateBlock(const glm::ivec3 &block, const EBlockType type) {
    updateBlock(block.x, block.y, block.z, type);
}

void Chunk::updateBlock(const int x, const int y, const int z, const EBlockType type) {
    if (!blocks[x][y][z].isNone() && type == BlockType_None) {
        activeBlockCount--;

    } else if (blocks[x][y][z].isNone() && type != BlockType_None) {
        activeBlockCount++;
    }

    blocks[x][y][z].blockType = type;
    _isDirty = true;
}

bool Chunk::shouldRender() const {
    return activeBlockCount != 0 && _isLoaded;
}

void Chunk::render(const OpenGLRenderer &renderer) {
    if (_isDirty) {
        createMesh(renderer.getTextureManager());
    }

    renderer.renderChunk(meshContext);
}

void Chunk::createMesh(const TextureManager &textureManager) {
    if (!meshContext) {
        throw std::runtime_error("tried to call createMesh() with no bound meshContext");
    }

    meshContext->modelTranslate = pos * CHUNK_SIZE;

    // mesh context is already empty so we can just start adding cubes
    blocks.forEach([&](const int x, const int y, const int z, const Block &b) {
        if (b.isNone()) return;
        createCube(x, y, z, textureManager);
    });

    meshContext->mergeQuads();
    meshContext->triangulateQuads();
    meshContext->makeIndexed();
    meshContext->writeToBuffers();
    meshContext->clear();
    _isDirty = false;
    isMesh = true;
}

void Chunk::createCube(const int x, const int y, const int z, const TextureManager &textureManager) {
    const glm::ivec3 cubePos = {x, y, z};
    const EBlockType blockType = blocks[x][y][z].blockType;

    if (z == CHUNK_SIZE - 1 || blocks[x][y][z + 1].isNone()) {
        createFace(cubePos, Front, blockType, textureManager);
    }

    if (z == 0 || blocks[x][y][z - 1].isNone()) {
        createFace(cubePos, Back, blockType, textureManager);
    }

    if (x == CHUNK_SIZE - 1 || blocks[x + 1][y][z].isNone()) {
        createFace(cubePos, Right, blockType, textureManager);
    }

    if (x == 0 || blocks[x - 1][y][z].isNone()) {
        createFace(cubePos, Left, blockType, textureManager);
    }

    if (y == CHUNK_SIZE - 1 || blocks[x][y + 1][z].isNone()) {
        createFace(cubePos, Top, blockType, textureManager);
    }

    if (y == 0 || blocks[x][y - 1][z].isNone()) {
        createFace(cubePos, Bottom, blockType, textureManager);
    }
}

void Chunk::createFace(const glm::ivec3 &cubePos, const EBlockFace face, const EBlockType blockType,
                       const TextureManager &textureManager) const {
    const auto [bottomLeft, topRight] = Block::getFaceCorners(face);
    const glm::ivec3 minPos = cubePos + bottomLeft;
    const glm::ivec3 maxPos = cubePos + topRight;

    const glm::vec3 normal = getNormalFromFace(face);
    const int samplerID = textureManager.getBlockSamplerID(blockType, face);

    const Vertex min{minPos, {0, 1}, normal, samplerID};
    const Vertex max{maxPos, {1, 0}, normal, samplerID};
    meshContext->addQuad(min, max);
}
