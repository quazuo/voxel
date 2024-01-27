#include "mesh-context.h"

#include <iostream>

ChunkMeshContext::ChunkMeshContext() :
    vertices(std::make_unique<GLArrayBuffer<glm::vec3>>(0, 3)),
    uvs(std::make_unique<GLArrayBuffer<glm::vec2>>(1, 2)),
    normals(std::make_unique<GLArrayBuffer<glm::vec3>>(2, 3)),
    texIDs(std::make_unique<GLArrayBuffer<int>>(3, 1)),
    indices(std::make_unique<GLElementBuffer>()) {}

void ChunkMeshContext::clear() {
    quads = {};
    triangles = {};
    indexedData = {};
}

void ChunkMeshContext::addQuad(const PackedVertex &min, const PackedVertex &max) {
    quads.emplace_back(min, max);
}

void ChunkMeshContext::addTriangle(const PackedVertex &vertex1, const PackedVertex &vertex2,
                                   const PackedVertex &vertex3) {
    triangles.emplace_back(vertex1, vertex2, vertex3);
}

static void indexVertex(const PackedVertex &vertex, IndexedMeshData &data,
                        std::map<PackedVertex, unsigned short> &vertexToOutIndex) {
    int index = -1;
    auto it = vertexToOutIndex.find(vertex);
    if (it != vertexToOutIndex.end()) {
        index = it->second;
    }

    bool found = index != -1;

    if (found) { // A similar vertex is already in the VBO, use it instead!
        data.indices.push_back(index);

    } else { // If not, it needs to be added in the output data.
        data.vertices.push_back(vertex.position);
        data.uvs.push_back(vertex.uv);
        data.normals.push_back(vertex.normal);
        data.texIDs.push_back(vertex.texSamplerID);
        unsigned short newIndex = (unsigned short) data.vertices.size() - 1;
        data.indices.push_back(newIndex);
        vertexToOutIndex[vertex] = newIndex;
    }
}

void ChunkMeshContext::makeIndexed() {
    if (indexedData)
        return;

    indexedData = IndexedMeshData();
    std::map<PackedVertex, unsigned short> vertexToOutIndex;

    for (const auto &[v1, v2, v3]: triangles) {
        indexVertex(v1, *indexedData, vertexToOutIndex);
        indexVertex(v2, *indexedData, vertexToOutIndex);
        indexVertex(v3, *indexedData, vertexToOutIndex);
    }
}

void ChunkMeshContext::writeToBuffers() {
    if (!indexedData) {
        throw std::runtime_error("tried to call writeToBuffers() without prior indexing");
    }

    vertices->write(indexedData->vertices);
    uvs->write(indexedData->uvs);
    normals->write(indexedData->normals);
    texIDs->write(indexedData->texIDs);
    indices->write(indexedData->indices);
}

void ChunkMeshContext::drawElements() {
    vertices->enable();
    uvs->enable();
    normals->enable();
    texIDs->enable();

    indices->enable();
    glDrawElements(GL_TRIANGLES, (GLsizei) indexedData->indices.size(), GL_UNSIGNED_SHORT, nullptr);

    vertices->disable();
    uvs->disable();
    normals->disable();
    texIDs->disable();
}

void ChunkMeshContext::triangulateQuads() {
    for (auto &[v1, v2]: quads) {
        PackedVertex v3 = {
            .position = {},
            .uv = {v2.uv.x, v1.uv.y},
            .normal = v1.normal,
            .texSamplerID = v1.texSamplerID,
        };

        PackedVertex v4 = {
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
    /*
     * front[x][y][z] == -1 iff there's no quad facing the {0, 0, 1} normal at these coords,
     * a non-zero value is the ID of the texture used by the quad. other ones work analogically.
     */
    CubeArray<short, Chunk::CHUNK_SIZE> front{-1}, back{-1}, right{-1}, left{-1}, top{-1}, bottom{-1};

    for (auto &[v1, v2]: quads) {
        const auto face = getFaceFromNormal(v1.normal);
        const auto cornerOffsets = Chunk::getFaceCorners(face);
        VecUtils::Vec3Discrete coords = v1.position - cornerOffsets.first;

        if (face == Front) {
            front[coords] = v1.texSamplerID;

        } else if (face == Back) {
            back[coords] = v1.texSamplerID;

        } else if (face == Right) {
            right[coords] = v1.texSamplerID;

        } else if (face == Left) {
            left[coords] = v1.texSamplerID;

        } else if (face == Top) {
            top[coords] = v1.texSamplerID;

        } else { // face == Bottom
            bottom[coords] = v1.texSamplerID;
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
}

std::vector<ChunkMeshContext::Quad>
ChunkMeshContext::mergeQuadMap(CubeArray<short, Chunk::CHUNK_SIZE> &quadMap, glm::vec3 normal) {
    std::vector<Quad> newQuads;

    const auto merge = [&](size_t x, size_t y, size_t z) {
        const VecUtils::Vec3Discrete firstAxis = normal.x == 0
                                                 ? glm::vec3(1, 0, 0)
                                                 : glm::vec3(0, 1, 0);
        const VecUtils::Vec3Discrete secondAxis = normal.z == 0
                                                  ? glm::vec3(0, 0, 1)
                                                  : glm::vec3(0, 1, 0);

        // first, stride along the first axis and determine the "width" of the merged rectangle
        VecUtils::Vec3Discrete firstStride = {x, y, z};
        bool isInRange = true;
        while (isInRange && quadMap[firstStride] == quadMap[x][y][z]) {
            firstStride += firstAxis;
            isInRange = VecUtils::all(firstStride, [](float f) { return f < Chunk::CHUNK_SIZE; });
        }

        const int width = VecUtils::sum(firstStride - VecUtils::Vec3Discrete(x, y, z));

        // second, stride along the second axis and determine the "height" of the merged rectangle
        VecUtils::Vec3Discrete secondStride = {x, y, z};
        isInRange = true;
        while (isInRange) {
            VecUtils::Vec3Discrete subStride = secondStride;
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
            isInRange = VecUtils::all(secondStride, [](float f) { return f < Chunk::CHUNK_SIZE; });
        }

        const int height = VecUtils::sum(secondStride - VecUtils::Vec3Discrete(x, y, z));

        VecUtils::Vec3Discrete v1 = {x, y, z};
        VecUtils::Vec3Discrete v2 = v1 + (firstAxis * (width - 1)) + (secondAxis * (height - 1));

        const auto face = getFaceFromNormal(normal);
        auto corners = Chunk::getFaceCorners(face);

        // these adjustments are needed if we want to stick with the same vertex indexing, and if we want to
        // keep the merging algorithm to one implementation.
        if (face == Top || face == Right) {
            std::swap(v1.z, v2.z);
        } else if (face == Back) {
            std::swap(v1.x, v2.x);
        }

        Quad q = {
            {
                .position = glm::vec3(v1) + corners.first,
                .uv = glm::vec2(0, 1) * (float) (face == Left || face == Right ? width : height),
                .normal = normal,
                .texSamplerID = quadMap[x][y][z]
            },
            {
                .position = glm::vec3(v2) + corners.second,
                .uv = glm::vec2(1, 0) * (float) (face == Left || face == Right ? height : width),
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
                quadMap[v1 + (firstAxis * i) + (secondAxis * j)] = -1;
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
