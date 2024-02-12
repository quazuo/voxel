#ifndef VOXEL_GL_BUFFER_H
#define VOXEL_GL_BUFFER_H

#include <cstddef>
#include "GL/glew.h"
#include <vector>
#include <glm/vec2.hpp>

/**
 * Abstraction over an OpenGL buffer, making it easier to manage by hiding all the OpenGL API calls.
 * This only abstracts buffers like array buffers and element buffers, which are used to be written to.
 * This is an abstract class that only defines some core functionalities, like writing and enabling the buffer.
 *
 * @tparam T Type of elements stored in the buffer.
 */
template<typename T>
class GLBuffer {
protected:
    /// The buffers always store `BASE_CAPACITY * 2^N` elements for some `N` depending on how many elements
    /// are being stored at the moment.
    static constexpr size_t BASE_CAPACITY = 9;
    GLsizeiptr capacity = BASE_CAPACITY;
    GLsizeiptr size = 0;

    GLuint bufferID {};

public:
    virtual ~GLBuffer();

    [[nodiscard]]
    decltype(size) getSize() const { return size; }

    /**
     * Writes the given data to the buffer, possibly reallocating it to contain all the data.
     *
     * @param data The data to be copied.
     */
    virtual void write(const std::vector<T> &data) = 0;

    /**
     * Enables the buffer. Used while rendering to inform OpenGL that we're using this buffer.
     */
    virtual void enable() = 0;

protected:
    /**
     * Adjusts the buffer's capacity to fit at least `dataSize` elements.
     *
     * @param dataSize Number of elements the buffer should be able to hold after calling this function.
     */
    void updateBufferCapacity(GLsizeiptr dataSize);
};

/**
 * A specialization over the GLBuffer class, using OpenGL's GL_ARRAY_BUFFER type.
 * Used primarily for storing stuff like vertex positions or normals.
 *
 * @tparam T Type of elements stored in the buffer.
 */
template<typename T>
class GLArrayBuffer final : public GLBuffer<T> {
    GLuint bufferIndex {};
    GLint compCount {};

public:
    GLArrayBuffer(GLuint index, GLint count);

    void write(const std::vector<T> &data) override;

    void enable() override;

    void disable() const;
};

/**
 * A specialization over the GLBuffer class, using OpenGL's GL_ELEMENT_BUFFER type.
 * Used for storing indices for indexed vertices. As such, it's restricted to holding specifically
 * elements of type `unsigned short`.
 */
class GLElementBuffer final : public GLBuffer<unsigned short> {
    using elemType = unsigned short;

public:
    GLElementBuffer();

    void write(const std::vector<elemType> &data) override;

    void enable() override;
};

class GLFrameBuffer final {
    GLuint bufferID {};

    // attachments
    GLuint texture {};
    GLuint depth {};
    GLuint stencil {};
    GLuint depthStencil {};

public:
    GLFrameBuffer();

    ~GLFrameBuffer();

    [[nodiscard]]
    GLuint getTexture() const { return texture; }

    [[nodiscard]]
    GLuint getDepth() const { return depth; }

    void enable() const;

    void disable() const;

    void attachTexture(glm::ivec2 size);

    void attachDepth(glm::ivec2 size);

    void attachDepthStencil(glm::ivec2 size);
};

#endif //VOXEL_GL_BUFFER_H
