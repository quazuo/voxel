#ifndef GL_VAO_H
#define GL_VAO_H

#include <memory>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "gl-buffer.h"
#include "src/utils/vec.h"

struct IndexedMeshData;

/**
 * Abstraction over an OpenGL Vertex Array Object. This is mainly used to hide API calls
 * to OpenGL for easier management and centralized implementation.
 */
class GLVertexArray {
protected:
    GLuint objectID {};

public:
    GLVertexArray();

    virtual ~GLVertexArray();

    /**
     * Enables the VAO. Used while rendering to inform OpenGL that we're using this VAO.
     */
    virtual void enable();
};

/**
 * Specialization of the GLVertexArray class, used for the purpose of holding all the data
 * relevant for rendering a single chunk's mesh in one VAO.
 */
class ChunkVertexArray final : public GLVertexArray {
    // std::unique_ptr<GLArrayBuffer<glm::uint32>> packedVertices;
    std::unique_ptr<GLArrayBuffer<glm::ivec3>> vertices;
    std::unique_ptr<GLArrayBuffer<glm::vec3>> normals;
    std::unique_ptr<GLArrayBuffer<glm::vec2>> uvs;
    std::unique_ptr<GLArrayBuffer<int>> texIDs;
    std::unique_ptr<GLElementBuffer> indices;

public:
    ChunkVertexArray();

    void writeToBuffers(const IndexedMeshData& data) const;
};

/**
 * Basic specialization of the GLVertexArray class for various things which don't require
 * anything apart from a list of vertices to be rendered, like outlines or the skybox.
 */
class BasicVertexArray final : public GLVertexArray {
    std::unique_ptr<GLArrayBuffer<glm::vec3>> vertices;

public:
    BasicVertexArray();

    void writeToBuffers(const std::vector<glm::vec3>& data) const;
};

#endif //GL_VAO_H
