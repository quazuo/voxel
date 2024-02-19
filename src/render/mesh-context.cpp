#include "mesh-context.h"

#include <iostream>

#include "gl/gl-vao.h"

std::vector<glm::uint32> IndexedMeshData::packNormalUvTex() const {
    std::vector<glm::uint32> packedData;

    for (size_t i = 0; i < vertices.size(); i++) {
        glm::uint32 packedVertex = 0;

        const glm::uint32 faceIndex = getFaceIndex(getFaceFromNormal(normals[i]));
        packedVertex |= (faceIndex & 0x7) << 29;

        packedVertex |= (static_cast<glm::uint32>(uvs[i].x) & 0x1F) << 24;
        packedVertex |= (static_cast<glm::uint32>(uvs[i].y) & 0x1F) << 19;

        packedVertex |= static_cast<glm::uint32>(texIDs[i]) & 0xFF;

        packedData.push_back(packedVertex);
    }

    return packedData;
}

void ChunkMeshContext::clear() {
    quads.clear();
    triangles.clear();
    indexedData = {};
}

void ChunkMeshContext::addQuad(const Vertex &min, const Vertex &max) {
    quads.emplace_back(min, max);
}

void ChunkMeshContext::addTriangle(const Vertex &vertex1, const Vertex &vertex2,
                                   const Vertex &vertex3) {
    triangles.emplace_back(vertex1, vertex2, vertex3);
}

static void indexVertex(const Vertex &vertex, IndexedMeshData &data,
                        std::map<Vertex, GLElementBuffer::ElemType> &vertexToOutIndex) {
    const auto it = vertexToOutIndex.find(vertex);

    if (it != vertexToOutIndex.end()) {
        // A similar vertex is already indexed, use that index instead!
        data.indices.push_back(it->second);

    } else {
        // If not, it needs to be added.
        data.vertices.push_back(vertex.position);
        data.uvs.push_back(vertex.uv);
        data.normals.push_back(vertex.normal);
        data.texIDs.push_back(vertex.texSamplerID);

        if (data.vertices.size() - 1 >= std::numeric_limits<GLElementBuffer::ElemType>::max()) {
            throw std::runtime_error("Detected overflow during mesh indexing. Please use a larger type for indices");
        }

        const auto newIndex = static_cast<GLElementBuffer::ElemType>(data.vertices.size() - 1);
        data.indices.push_back(newIndex);
        vertexToOutIndex.emplace(vertex, newIndex);
    }
}

void ChunkMeshContext::makeIndexed() {
    indexedData = IndexedMeshData();
    std::map<Vertex, GLElementBuffer::ElemType> vertexToOutIndex;

    for (const auto &[v1, v2, v3]: triangles) {
        indexVertex(v1, *indexedData, vertexToOutIndex);
        indexVertex(v2, *indexedData, vertexToOutIndex);
        indexVertex(v3, *indexedData, vertexToOutIndex);
    }
}

void ChunkMeshContext::triangulateQuads() {
    for (auto &[v1, v2]: quads) {
        Vertex v3 = {
            .position = {},
            .uv = {v2.uv.x, v1.uv.y},
            .normal = v1.normal,
            .texSamplerID = v1.texSamplerID,
        };

        Vertex v4 = {
            .position = {},
            .uv = {v1.uv.x, v2.uv.y},
            .normal = v1.normal,
            .texSamplerID = v1.texSamplerID,
        };

        if (v1.position.x == v2.position.x) {
            v3.position = {v1.position.x, v1.position.y, v2.position.z};
            v4.position = {v1.position.x, v2.position.y, v1.position.z};

        } else if (v1.position.y == v2.position.y) {
            v3.position = {v2.position.x, v1.position.y, v1.position.z};
            v4.position = {v1.position.x, v1.position.y, v2.position.z};

        } else if (v1.position.z == v2.position.z) {
            v3.position = {v2.position.x, v1.position.y, v1.position.z};
            v4.position = {v1.position.x, v2.position.y, v1.position.z};

        } else {
            throw std::runtime_error("invalid axis alignment in triangulateQuads()");
        }

        triangles.emplace_back(v1, v3, v2);
        triangles.emplace_back(v1, v2, v4);
    }
}

void ChunkMeshContext::mergeQuads() {
    if (quads.empty()) return;

    /*
     * `front[x][y][z] == -1` iff there's no quad facing the [0, 0, 1] normal at [x, y, z] coords,
     * a non-zero value is the ID of the texture used by the quad. other ones work analogically.
     */
    CubeArray<short, Chunk::CHUNK_SIZE> front{-1}, back{-1}, right{-1}, left{-1}, top{-1}, bottom{-1};

    for (auto &[v1, v2]: quads) {
        const EBlockFace face = getFaceFromNormal(v1.normal);
        const glm::ivec3 absCoords = v1.position - Block::getFaceCorners(face).first;
        const glm::ivec3 relCoords = absCoords - glm::ivec3(modelTranslate);

        if (face == Front) {
            front[relCoords] = v1.texSamplerID;

        } else if (face == Back) {
            back[relCoords] = v1.texSamplerID;

        } else if (face == Right) {
            right[relCoords] = v1.texSamplerID;

        } else if (face == Left) {
            left[relCoords] = v1.texSamplerID;

        } else if (face == Top) {
            top[relCoords] = v1.texSamplerID;

        } else { // face == Bottom
            bottom[relCoords] = v1.texSamplerID;
        }
    }

    quads = {};

    const auto newFrontQuads = mergeQuadMap(front, getNormalFromFace(Front));
    quads.insert(quads.end(), newFrontQuads.begin(), newFrontQuads.end());

    const auto newBackQuads = mergeQuadMap(back, getNormalFromFace(Back));
    quads.insert(quads.end(), newBackQuads.begin(), newBackQuads.end());

    const auto newRightQuads = mergeQuadMap(right, getNormalFromFace(Right));
    quads.insert(quads.end(), newRightQuads.begin(), newRightQuads.end());

    const auto newLeftQuads = mergeQuadMap(left, getNormalFromFace(Left));
    quads.insert(quads.end(), newLeftQuads.begin(), newLeftQuads.end());

    const auto newTopQuads = mergeQuadMap(top, getNormalFromFace(Top));
    quads.insert(quads.end(), newTopQuads.begin(), newTopQuads.end());

    const auto newBottomQuads = mergeQuadMap(bottom, getNormalFromFace(Bottom));
    quads.insert(quads.end(), newBottomQuads.begin(), newBottomQuads.end());

    for (auto& [first, second]: quads) {
        first.position += modelTranslate;
        second.position += modelTranslate;
    }
}

std::vector<ChunkMeshContext::Quad>
ChunkMeshContext::mergeQuadMap(CubeArray<short, Chunk::CHUNK_SIZE> &quadMap, const glm::vec3 &normal) {
    std::vector<Quad> newQuads;

    const auto merge = [&](size_t x, size_t y, size_t z) {
        const glm::ivec3 firstAxis = normal.x == 0
                                                 ? glm::vec3(1, 0, 0)
                                                 : glm::vec3(0, 1, 0);
        const glm::ivec3 secondAxis = normal.z == 0
                                                  ? glm::vec3(0, 0, 1)
                                                  : glm::vec3(0, 1, 0);

        // first, stride along the first axis and determine the "width" of the merged rectangle
        glm::ivec3 firstStride = {x, y, z};
        bool isInRange = true;
        while (isInRange && quadMap[firstStride] == quadMap[x][y][z]) {
            firstStride += firstAxis;
            isInRange = VecUtils::all<float>(firstStride, [](const float f) { return f < Chunk::CHUNK_SIZE; });
        }

        const int width = VecUtils::sum(firstStride - glm::ivec3(x, y, z));

        // second, stride along the second axis and determine the "height" of the merged rectangle
        glm::ivec3 secondStride = {x, y, z};
        isInRange = true;
        while (isInRange) {
            glm::ivec3 subStride = secondStride;
            bool isRowMatching = true;

            for (int i = 0; i < width; i++) {
                if (quadMap[subStride] != quadMap[x][y][z]) {
                    isRowMatching = false;
                    break;
                }

                subStride += firstAxis;
            }

            if (!isRowMatching)
                break;

            secondStride += secondAxis;
            isInRange = VecUtils::all<float>(secondStride, [](const float f) { return f < Chunk::CHUNK_SIZE; });
        }

        const int height = VecUtils::sum(secondStride - glm::ivec3(x, y, z));

        glm::ivec3 v1 = {x, y, z};
        glm::ivec3 v2 = v1 + firstAxis * (width - 1) + secondAxis * (height - 1);

        const auto face = getFaceFromNormal(normal);
        const auto [bottomLeft, topRight] = Block::getFaceCorners(face);

        // these adjustments are needed if we want to stick with the same vertex indexing, and if we want to
        // keep the merging algorithm to one implementation.
        if (face == Top || face == Right) {
            std::swap(v1.z, v2.z);
        } else if (face == Back) {
            std::swap(v1.x, v2.x);
        }

        Quad q = {
            {
                .position = v1 + bottomLeft,
                .uv = glm::ivec2(0, 1) * (face == Left || face == Right ? width : height),
                .normal = normal,
                .texSamplerID = quadMap[x][y][z]
            },
            {
                .position = v2 + topRight,
                .uv = glm::ivec2(1, 0) * (face == Left || face == Right ? height : width),
                .normal = normal,
                .texSamplerID = quadMap[x][y][z]
            }
        };

        newQuads.push_back(q);

        // swap back so that the next loop works correctly
        if (face == Top || face == Right) {
            std::swap(v1.z, v2.z);
        } else if (face == Back) {
            std::swap(v1.x, v2.x);
        }

        // remove merged quads from the map
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                quadMap[v1 + firstAxis * i + secondAxis * j] = -1;
            }
        }
    };

    for (int x = 0; x < Chunk::CHUNK_SIZE; x++) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; y++) {
            for (int z = 0; z < Chunk::CHUNK_SIZE; z++) {
                if (quadMap[x][y][z] != -1)
                    merge(x, y, z);
            }
        }
    }

    return newQuads;
}
