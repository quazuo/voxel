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
    size_t capacity = BASE_CAPACITY;
    size_t size = 0;

    GLuint bufferID{};

public:
    virtual ~GLBuffer();

    [[nodiscard]]
    decltype(size) getSize() const { return size; }

    /**
     * Writes the given data to the buffer, possibly reallocating it to contain all the data.
     *
     * @param data The data to be copied.
     * @param count Number of elements to be copied.
     * @param offset Offset (in number of elements) from the start of the buffer at which writing should start.
     */
    virtual void write(const T* data, size_t count, size_t offset) = 0;

    /**
     * Enables the buffer. Used while rendering to inform OpenGL that we're using this buffer.
     */
    virtual void enable() = 0;

protected:
    /**
     * Adjusts the buffer's capacity to fit at least `dataSize` elements.
     *
     * @param desiredCapacity Number of elements the buffer should be able to hold after calling this function.
     */
    void updateBufferCapacity(size_t desiredCapacity);

    [[nodiscard]]
    virtual GLenum getGlTarget() const { return GL_ARRAY_BUFFER; }
};

/**
 * A specialization over the GLBuffer class, using OpenGL's GL_ARRAY_BUFFER type.
 * Used primarily for storing stuff like vertex positions or normals.
 *
 * @tparam T Type of elements stored in the buffer.
 */
template<typename T>
class GLArrayBuffer final : public GLBuffer<T> {
    GLuint bufferIndex{};
    GLint compCount{};

public:
    GLArrayBuffer(GLuint index, GLint count);

    void write(const T* data, size_t count, size_t offset) override;

    void enable() override;

    void disable() const;
};

/**
 * A specialization over the GLBuffer class, using OpenGL's GL_ELEMENT_BUFFER type.
 * Used for storing indices for indexed vertices.
 *
 * When the indices are kept as unsigned shorts, this leads to a problem, because in an edge case
 * a chunk with `CHUNK_SIZE == 16` may contain more than USHORT_MAX rendered vertices.
 * I'm currently ignoring these edge cases, as transparent blocks aren't implemented yet, and due to the
 * fact that I can't come up with a pattern that would make this buffer overflow, I just assume there isn't one.
 * hopefully, the problem will just disappear in the future when I come up with some more elaborate ways to
 * exclude unseen blocks from rendering.
 */
class GLElementBuffer final : public GLBuffer<unsigned int> {
public:
    using ElemType = unsigned int;

    static unsigned int getGlElemType() { return GL_UNSIGNED_INT; }

    GLElementBuffer();

    void write(const ElemType* data, size_t count, size_t offset) override;

    void enable() override;

protected:
    [[nodiscard]]
    GLenum getGlTarget() const override { return GL_ELEMENT_ARRAY_BUFFER; }
};

/**
 * This doesn't derive from GLBuffer mostly because we don't really expect a framebuffer
 * to use the same interface as the buffers above, since these expect to be directly written to,
 * which framebuffers do not.
 *
 * So far, this implementation is VERY barebones and rudimentary. It's definitely subject for improvement.
 */
class GLFrameBuffer final {
    GLuint bufferID{};

    // attachments
    GLuint texture{};
    GLuint depth{};
    GLuint stencil{};
    GLuint depthStencil{};

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
