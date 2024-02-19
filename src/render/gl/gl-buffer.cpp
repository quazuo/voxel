#include "gl-buffer.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

#include <stdexcept>

template<typename T>
GLBuffer<T>::~GLBuffer() {
    glDeleteBuffers(1, &bufferID);
}

template<typename T>
void GLBuffer<T>::updateBufferCapacity(const size_t desiredCapacity) {
    if (desiredCapacity <= capacity) return;

    size_t newCapacity = capacity;

    while (desiredCapacity > newCapacity) {
        newCapacity *= 2;
    }

    capacity = newCapacity;

    const GLint target = getGlTarget();

    glBindBuffer(target, bufferID);

    static GLuint tempBuffer = 0;
    if (!tempBuffer) glGenBuffers(1, &tempBuffer);
    glBindBuffer(GL_COPY_WRITE_BUFFER, tempBuffer);
    glBufferData(GL_COPY_WRITE_BUFFER, size * sizeof(T), nullptr, GL_STATIC_COPY);

    glCopyBufferSubData(target, GL_COPY_WRITE_BUFFER, 0, 0, size * sizeof(T));

    glBufferData(target, capacity * sizeof(T), nullptr, GL_DYNAMIC_DRAW);

    glCopyBufferSubData(GL_COPY_WRITE_BUFFER, target, 0, 0, size * sizeof(T));
}

template<typename T>
GLArrayBuffer<T>::GLArrayBuffer(const GLuint index, const GLint count) : bufferIndex(index), compCount(count) {
    glGenBuffers(1, &this->bufferID);
    glBindBuffer(this->getGlTarget(), this->bufferID);
    glVertexAttribPointer(bufferIndex, compCount, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBufferData(this->getGlTarget(), this->BASE_CAPACITY * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(bufferIndex);
}

template<typename T>
void GLArrayBuffer<T>::write(const T* data, const size_t count, const size_t offset) {
    this->updateBufferCapacity(offset + count);
    this->size = std::max(this->size, offset + count);

    glBindBuffer(this->getGlTarget(), this->bufferID);
    glBufferSubData(
        this->getGlTarget(),
        static_cast<GLsizeiptr>(offset * sizeof(T)),
        static_cast<GLsizeiptr>(count * sizeof(T)),
        data
    );
}

template<typename T>
void GLArrayBuffer<T>::enable() {
    glBindBuffer(this->getGlTarget(), this->bufferID);
}

template<typename T>
void GLArrayBuffer<T>::disable() const {
    glDisableVertexAttribArray(bufferIndex);
}

GLElementBuffer::GLElementBuffer() {
    glGenBuffers(1, &bufferID);
    glBindBuffer(getGlTarget(), bufferID);
    glBufferData(getGlTarget(), BASE_CAPACITY * sizeof(ElemType), nullptr, GL_DYNAMIC_DRAW);
}

void GLElementBuffer::write(const ElemType* data, const size_t count, const size_t offset) {
    updateBufferCapacity(offset + count);
    size = std::max(size, offset + count);

    glBindBuffer(getGlTarget(), bufferID);
    glBufferSubData(
        getGlTarget(),
        static_cast<GLsizeiptr>(offset * sizeof(ElemType)),
        static_cast<GLsizeiptr>(count * sizeof(ElemType)),
        data
    );
}

void GLElementBuffer::enable() {
    glBindBuffer(getGlTarget(), bufferID);
}

GLFrameBuffer::GLFrameBuffer() {
    glGenFramebuffers(1, &bufferID);
}

GLFrameBuffer::~GLFrameBuffer() {
    glDeleteFramebuffers(1, &bufferID);
}

void GLFrameBuffer::enable() const {
    glBindFramebuffer(GL_FRAMEBUFFER, bufferID);
}

void GLFrameBuffer::disable() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBuffer::attachTexture(const glm::ivec2 size) {
    enable();

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    disable();
}

void GLFrameBuffer::attachDepth(const glm::ivec2 size) {
    glGenTextures(1, &depth);
    glBindTexture(GL_TEXTURE_2D, depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    enable();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    disable();
}

void GLFrameBuffer::attachDepthStencil(const glm::ivec2 size) {
    enable();

    glGenTextures(1, &depthStencil);
    glBindTexture(GL_TEXTURE_2D, depthStencil);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.x, size.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencil, 0);

    disable();
}

// template specializations

template class GLBuffer<glm::uvec3>;
template class GLBuffer<glm::ivec3>;
template class GLBuffer<glm::vec3>;
template class GLBuffer<glm::vec2>;
template class GLBuffer<glm::ivec2>;
template class GLBuffer<glm::uint32>;
template class GLBuffer<unsigned short>;
template class GLBuffer<int>;

template class GLArrayBuffer<glm::uvec3>;
template class GLArrayBuffer<glm::ivec3>;
template class GLArrayBuffer<glm::vec3>;
template class GLArrayBuffer<glm::vec2>;
template class GLArrayBuffer<glm::ivec2>;
template class GLArrayBuffer<glm::uint32>;
template class GLArrayBuffer<int>;
