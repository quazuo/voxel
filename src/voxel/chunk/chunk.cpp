#include "chunk.h"

#include "src/render/renderer.h"
#include "src/render/mesh-context.h"
#include "src/voxel/world-gen.h"

void Chunk::load() {
    _isLoaded = true;

    // todo - check if is stored on the disk and if it is, load it, otherwise generate terrain for it
    // todo - ^ this should probably be done in the chunk manager

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
}

void Chunk::unload() {
    meshContext->freeBuffers();
}

void Chunk::updateBlock(int x, int y, int z, EBlockType type) {
    blocks[x][y][z].blockType = type;
    _isDirty = true;
}

void Chunk::render(const std::shared_ptr<OpenGLRenderer>& renderer) {
    if (activeBlockCount == 0)
        return;

    if (_isDirty) {
        createMesh();
        _isDirty = false;
    }

    if (isMesh) {
        renderer->renderChunk(*meshContext);
        meshContext->isFreshlyUpdated = false;
    }
}

void Chunk::createMesh() {
    if (!meshContext) {
        meshContext = std::make_shared<MeshContext>(MeshContext());
        meshContext->modelTranslate = pos * (float) CHUNK_SIZE;
        meshContext->initBuffers();
    }

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
    glm::vec3 cubeIndexPos = {(double) x, (double) y, (double) z};
    glm::vec3 cubePos = cubeIndexPos * (float) Block::RENDER_SIZE * 2.0f;

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

    const float offset = Block::RENDER_SIZE;
    glm::vec3 p1(cubePos.x - offset, cubePos.y - offset, cubePos.z + offset);
    glm::vec3 p2(cubePos.x + offset, cubePos.y - offset, cubePos.z + offset);
    glm::vec3 p3(cubePos.x + offset, cubePos.y + offset, cubePos.z + offset);
    glm::vec3 p4(cubePos.x - offset, cubePos.y + offset, cubePos.z + offset);
    glm::vec3 p5(cubePos.x + offset, cubePos.y - offset, cubePos.z - offset);
    glm::vec3 p6(cubePos.x - offset, cubePos.y - offset, cubePos.z - offset);
    glm::vec3 p7(cubePos.x - offset, cubePos.y + offset, cubePos.z - offset);
    glm::vec3 p8(cubePos.x + offset, cubePos.y + offset, cubePos.z - offset);

    glm::vec3 normal;
    glm::vec2 uvOffset;
    EBlockType blockType = blocks[x][y][z].blockType;

    // front
    if (z == CHUNK_SIZE - 1 || blocks[x][y][z + 1].isNone()) {
        normal = {0.0, 0.0, 1.0};
        uvOffset = {0.0, 0.0};
        createFace(p1, p2, p3, p4, uvOffset, normal, blockType);
    }

    // back
    if (z == 0 || blocks[x][y][z - 1].isNone()) {
        normal = {0.0, 0.0, -1.0};
        uvOffset = {1.0 / 4, 0};
        createFace(p5, p6, p7, p8, uvOffset, normal, blockType);
    }

    // right
    if (x == CHUNK_SIZE - 1 || blocks[x + 1][y][z].isNone()) {
        normal = {1.0, 0.0, 0.0};
        uvOffset = {2.0 / 4, 0};
        createFace(p2, p5, p8, p3, uvOffset, normal, blockType);
    }

    // left
    if (x == 0 || blocks[x - 1][y][z].isNone()) {
        normal = {-1.0, 0.0, 0.0};
        uvOffset = {3.0 / 4, 0};
        createFace(p6, p1, p4, p7, uvOffset, normal, blockType);
    }

    // top
    if (y == CHUNK_SIZE - 1 || blocks[x][y + 1][z].isNone()) {
        normal = {0.0, 1.0, 0.0};
        uvOffset = {0, 1.0 / 4};
        createFace(p4, p3, p8, p7, uvOffset, normal, blockType);
    }

    // bottom
    if (y == 0 || blocks[x][y - 1][z].isNone()) {
        normal = {0.0, -1.0, 0.0};
        uvOffset = {1.0 / 4, 1.0 / 4};
        createFace(p6, p5, p2, p1, uvOffset, normal, blockType);
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

    const glm::vec2 uvTexOffset = {(double) blockType - 1, 0}; // -1 because actual blocks (apart from None) start at 1

    PackedVertex pv1 = {v1, uvs[0] + uvOffset + uvTexOffset, normal};
    PackedVertex pv2 = {v2, uvs[1] + uvOffset + uvTexOffset, normal};
    PackedVertex pv3 = {v3, uvs[2] + uvOffset + uvTexOffset, normal};
    PackedVertex pv4 = {v4, uvs[3] + uvOffset + uvTexOffset, normal};
    meshContext->addTriangle(pv1, pv2, pv3);
    meshContext->addTriangle(pv1, pv3, pv4);
}
