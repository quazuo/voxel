#include "chunk.h"

#include <libnoise/noise.h>
#include <iostream>

#include "src/render/renderer.h"
#include "deps/noiseutils/noiseutils.h"

void Chunk::load() {
    _isLoaded = true;
    // todo - check if is stored on the disk and if it is, load it, otherwise generate terrain for it
    // todo - ^ this should probably be done in the chunk manager
}

void Chunk::unload() {
    // todo
}

void Chunk::updateBlock(int x, int y, int z, EBlockType type) {
    blocks[x][y][z].blockType = type;
    isDirty = true;
}

void Chunk::render(OpenGLRenderer &renderer) {
//    auto [x, y, z] = pos;
//    renderer->translateWorldMatrix(x, y, z);

    if (isDirty) {
        noise::module::Perlin noiseModule;
        noiseutils::NoiseMap heightMap;
        noiseutils::NoiseMapBuilderPlane heightMapBuilder;
        heightMapBuilder.SetSourceModule(noiseModule);
        heightMapBuilder.SetDestNoiseMap(heightMap);
        heightMapBuilder.SetDestSize(CHUNK_SIZE, CHUNK_SIZE);
        heightMapBuilder.SetBounds(pos.x, pos.x + 2.0, pos.z, pos.z + 2.0);
        heightMapBuilder.Build();

        // remove some blocks to see what it looks like without them
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                float height = (float) CHUNK_SIZE / 2 + std::clamp(
                    heightMap.GetValue(x, z) * CHUNK_SIZE,
                    (float) -CHUNK_SIZE / 2 + 1,
                    (float) CHUNK_SIZE / 2
                );

                for (int y = 0; y < CHUNK_SIZE; y++) {
                    blocks[x][y][z].blockType = y < height
                        ? EBlockType::BlockType_Grass
                        : EBlockType::BlockType_None;
                }
            }
        }

        createMesh(renderer);
        isDirty = false;
    }

    if (isMesh) {
        renderer.renderMesh(*meshContext);
    }
}

void Chunk::createMesh(OpenGLRenderer &renderer) {
    (void) renderer; // it's unused for now

    meshContext = std::make_shared<MeshContext>(MeshContext());

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (blocks[x][y][z].isNone())
                    continue;

                createCube(x, y, z);
            }
        }
    }

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

    // front
    if (z == CHUNK_SIZE - 1 || blocks[x][y][z + 1].isNone()) {
        normal = {0.0, 0.0, 1.0};
        offset = {0.0, 0.0};
        createFace(p1, p2, p3, p4, offset, normal);
    }

    // back
    if (z == 0 || blocks[x][y][z - 1].isNone()) {
        normal = {0.0, 0.0, -1.0};
        offset = {1.0 / 4, 0};
        createFace(p5, p6, p7, p8, offset, normal);
    }

    // right
    if (x == CHUNK_SIZE - 1 || blocks[x + 1][y][z].isNone()) {
        normal = {1.0, 0.0, 0.0};
        offset = {2.0 / 4, 0};
        createFace(p2, p5, p8, p3, offset, normal);
    }

    // left
    if (x == 0 || blocks[x - 1][y][z].isNone()) {
        normal = {-1.0, 0.0, 0.0};
        offset = {3.0 / 4, 0};
        createFace(p6, p1, p4, p7, offset, normal);
    }

    // top
    if (y == CHUNK_SIZE - 1 || blocks[x][y + 1][z].isNone()) {
        normal = {0.0, 1.0, 0.0};
        offset = {0, 1.0 / 4};
        createFace(p4, p3, p8, p7, offset, normal);
    }

    // bottom
    if (y == 0 || blocks[x][y - 1][z].isNone()) {
        normal = {0.0, -1.0, 0.0};
        offset = {1.0 / 4, 1.0 / 4};
        createFace(p6, p5, p2, p1, offset, normal);
    }
}

void Chunk::createFace(const Vec3 v1, const Vec3 v2, const Vec3 v3, const Vec3 v4,
                       const Vec2 uvOffset, const Vec3 normal) {
    // UV coordinates of the front face, used as a reference point for other faces
    constexpr Vec2 uvs[4] = {
        {0,       1.0 / 4},
        {1.0 / 4, 1.0 / 4},
        {1.0 / 4, 0},
        {0,       0}
    };

    PackedVertex pv1 = {v1, uvs[0] + uvOffset, normal};
    PackedVertex pv2 = {v2, uvs[1] + uvOffset, normal};
    PackedVertex pv3 = {v3, uvs[2] + uvOffset, normal};
    PackedVertex pv4 = {v4, uvs[3] + uvOffset, normal};
    meshContext->addTriangle(pv1, pv2, pv3);
    meshContext->addTriangle(pv1, pv3, pv4);
}