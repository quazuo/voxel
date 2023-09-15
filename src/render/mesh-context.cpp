#include "mesh-context.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

void MeshContext::clear() {
    triangles = {};
    indexedData = {};
    isIndexed = false;
}

void MeshContext::addTriangle(PackedVertex &vertex1, PackedVertex &vertex2, PackedVertex &vertex3) {
    triangles.emplace_back(vertex1, vertex2, vertex3);
}

void MeshContext::indexVertex(const PackedVertex &vertex, IndexedMeshData &data,
                              std::map<PackedVertex, unsigned short> &vertexToOutIndex) {
    int index = -1;
    auto it = vertexToOutIndex.find(vertex);
    if (it != vertexToOutIndex.end())
        index = it->second;

    bool found = index != -1;

    if (found) { // A similar vertex is already in the VBO, use it instead!
        data.indices.push_back(index);

    } else { // If not, it needs to be added in the output data.
        data.vertices.push_back(vertex.position);
        data.uvs.push_back(vertex.uv);
        data.normals.push_back(vertex.normal);
        unsigned short newIndex = (unsigned short) data.vertices.size() - 1;
        data.indices.push_back(newIndex);
        vertexToOutIndex[vertex] = newIndex;
    }
}

void MeshContext::makeIndexed() {
    if (isIndexed)
        return;

    std::map<PackedVertex, unsigned short> vertexToOutIndex;

    for (const auto &[v1, v2, v3]: triangles) {
        indexVertex(v1, indexedData, vertexToOutIndex);
        indexVertex(v2, indexedData, vertexToOutIndex);
        indexVertex(v3, indexedData, vertexToOutIndex);
    }

    isIndexed = true;
}

void MeshContext::initBuffers() {
    vertices.init(0, 3);
    uvs.init(1, 2);
    normals.init(2, 3);
    indices.init();
}

void MeshContext::writeToBuffers() {
    if (!isIndexed) {
        throw std::runtime_error("tried to call writeToBuffers() without prior indexing");
    }

    vertices.write(indexedData.vertices);
    uvs.write(indexedData.uvs);
    normals.write(indexedData.normals);
    indices.write(indexedData.indices);
}

void MeshContext::enableArrayBuffers() {
    vertices.enable();
    uvs.enable();
    normals.enable();
}

void MeshContext::disableArrayBuffers() {
    vertices.disable();
    uvs.disable();
    normals.disable();
}

void MeshContext::freeBuffers() {
    vertices.free();
    uvs.free();
    normals.free();
    indices.free();
}

void MeshContext::drawElements() {
    indices.enable();
    glDrawElements(GL_TRIANGLES, indexedData.indices.size(), GL_UNSIGNED_SHORT, nullptr);
}
