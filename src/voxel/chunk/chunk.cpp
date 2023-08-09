#include "chunk.h"

#include <iostream>

#include "src/render/renderer.h"
#include "deps/noiseutils/noiseutils.h"
#include "src/voxel/world-gen.h"

void Chunk::load() {
    _isLoaded = true;

    // todo - check if is stored on the disk and if it is, load it, otherwise generate terrain for it
    // todo - ^ this should probably be done in the chunk manager

    // remove some blocks to see what it looks like without them
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            float height = (float) CHUNK_SIZE / 2 + std::clamp(
                WorldGen::getHeight(pos, x, z) * CHUNK_SIZE / 2,
                (float) -CHUNK_SIZE / 2 + 1,
                (float) CHUNK_SIZE / 2
            );

            for (int y = 0; y < CHUNK_SIZE; y++) {
                blocks[x][y][z].blockType = y < height
                    ? EBlockType::BlockType_Dirt
                    : EBlockType::BlockType_None;
            }
        }
    }

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (blocks[x][y][z].blockType != EBlockType::BlockType_Dirt)
                    continue;
                if (y != CHUNK_SIZE - 1 && blocks[x][y + 1][z].blockType != EBlockType::BlockType_None)
                    continue;
                blocks[x][y][z].blockType = EBlockType::BlockType_Grass;
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
        meshContext->modelTranslate = pos * CHUNK_SIZE;
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
    Vec3 cubeIndexPos = {(double) x, (double) y, (double) z};
    Vec3 cubePos = cubeIndexPos * Block::RENDER_SIZE * 2;
    auto [xx, yy, zz] = cubePos;

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

    Vec3 p1(xx - Block::RENDER_SIZE, yy - Block::RENDER_SIZE, zz + Block::RENDER_SIZE);
    Vec3 p2(xx + Block::RENDER_SIZE, yy - Block::RENDER_SIZE, zz + Block::RENDER_SIZE);
    Vec3 p3(xx + Block::RENDER_SIZE, yy + Block::RENDER_SIZE, zz + Block::RENDER_SIZE);
    Vec3 p4(xx - Block::RENDER_SIZE, yy + Block::RENDER_SIZE, zz + Block::RENDER_SIZE);
    Vec3 p5(xx + Block::RENDER_SIZE, yy - Block::RENDER_SIZE, zz - Block::RENDER_SIZE);
    Vec3 p6(xx - Block::RENDER_SIZE, yy - Block::RENDER_SIZE, zz - Block::RENDER_SIZE);
    Vec3 p7(xx - Block::RENDER_SIZE, yy + Block::RENDER_SIZE, zz - Block::RENDER_SIZE);
    Vec3 p8(xx + Block::RENDER_SIZE, yy + Block::RENDER_SIZE, zz - Block::RENDER_SIZE);

    Vec3 normal;
    Vec2 offset;
    EBlockType blockType = blocks[x][y][z].blockType;

    // front
    if (z == CHUNK_SIZE - 1 || blocks[x][y][z + 1].isNone()) {
        normal = {0.0, 0.0, 1.0};
        offset = {0.0, 0.0};
        createFace(p1, p2, p3, p4, offset, normal, blockType);
    }

    // back
    if (z == 0 || blocks[x][y][z - 1].isNone()) {
        normal = {0.0, 0.0, -1.0};
        offset = {1.0 / 4, 0};
        createFace(p5, p6, p7, p8, offset, normal, blockType);
    }

    // right
    if (x == CHUNK_SIZE - 1 || blocks[x + 1][y][z].isNone()) {
        normal = {1.0, 0.0, 0.0};
        offset = {2.0 / 4, 0};
        createFace(p2, p5, p8, p3, offset, normal, blockType);
    }

    // left
    if (x == 0 || blocks[x - 1][y][z].isNone()) {
        normal = {-1.0, 0.0, 0.0};
        offset = {3.0 / 4, 0};
        createFace(p6, p1, p4, p7, offset, normal, blockType);
    }

    // top
    if (y == CHUNK_SIZE - 1 || blocks[x][y + 1][z].isNone()) {
        normal = {0.0, 1.0, 0.0};
        offset = {0, 1.0 / 4};
        createFace(p4, p3, p8, p7, offset, normal, blockType);
    }

    // bottom
    if (y == 0 || blocks[x][y - 1][z].isNone()) {
        normal = {0.0, -1.0, 0.0};
        offset = {1.0 / 4, 1.0 / 4};
        createFace(p6, p5, p2, p1, offset, normal, blockType);
    }
}

void Chunk::createFace(const Vec3 v1, const Vec3 v2, const Vec3 v3, const Vec3 v4,
                       const Vec2 uvOffset, const Vec3 normal, const EBlockType blockType) {
    // UV coordinates of the front face, used as a reference point for other faces
    constexpr Vec2 uvs[4] = {
        {0,       1.0 / 4},
        {1.0 / 4, 1.0 / 4},
        {1.0 / 4, 0},
        {0,       0}
    };

    const Vec2 uvTexOffset = {(double) blockType - 1, 0}; // -1 because actual blocks (apart from None) start at 1

    PackedVertex pv1 = {v1, uvs[0] + uvOffset + uvTexOffset, normal};
    PackedVertex pv2 = {v2, uvs[1] + uvOffset + uvTexOffset, normal};
    PackedVertex pv3 = {v3, uvs[2] + uvOffset + uvTexOffset, normal};
    PackedVertex pv4 = {v4, uvs[3] + uvOffset + uvTexOffset, normal};
    meshContext->addTriangle(pv1, pv2, pv3);
    meshContext->addTriangle(pv1, pv3, pv4);
}
