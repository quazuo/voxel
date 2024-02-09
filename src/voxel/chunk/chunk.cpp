#include "chunk.h"

#include "src/render/renderer.h"
#include "src/render/mesh-context.h"
#include "src/voxel/world-gen.h"

void Chunk::generate(const std::shared_ptr<WorldGen> &worldGen) {
    worldGen->setChunkGenCtx(pos);

    blocks.forEach([&](const int x, const int y, const int z, const Block &b) {
        (void) b;
        updateBlock(x, y, z, worldGen->getBlockTypeAt({x, y, z}));
    });

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

void Chunk::render(const std::shared_ptr<OpenGLRenderer> &renderer) {
    if (activeBlockCount == 0)
        return;

    if (_isDirty) {
        createMesh(renderer->getTextureManager());
        _isDirty = false;
    }

    if (isMesh) {
        renderer->renderChunk(meshContext);
    }
}

void Chunk::createMesh(const TextureManager &textureManager) {
    if (!meshContext) {
        throw std::runtime_error("tried to call createMesh() with no bound meshContext");
    }

    meshContext->clear();
    meshContext->modelTranslate = pos * CHUNK_SIZE;

    blocks.forEach([&](const int x, const int y, const int z, const Block &b) {
        if (b.isNone()) return;
        createCube(x, y, z, textureManager);
    });

    meshContext->mergeQuads();
    meshContext->triangulateQuads();
    meshContext->makeIndexed();
    meshContext->isFreshlyUpdated = true;
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
    static const std::unordered_map<EBlockFace, glm::vec3> faceNormals = {
        {Front, {0, 0, 1}},
        {Back, {0, 0, -1}},
        {Right, {1, 0, 0}},
        {Left, {-1, 0, 0}},
        {Top, {0, 1, 0}},
        {Bottom, {0, -1, 0}}
    };

    const auto [bottomLeft, topRight] = Block::getFaceCorners(face);
    const glm::ivec3 minPos = cubePos + bottomLeft;
    const glm::ivec3 maxPos = cubePos + topRight;

    const glm::vec3 normal = faceNormals.at(face);
    const int samplerID = textureManager.getBlockSamplerID(blockType, face);

    const Vertex min{minPos, {0, 1}, normal, samplerID};
    const Vertex max{maxPos, {1, 0}, normal, samplerID};
    meshContext->addQuad(min, max);
}
