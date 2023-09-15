#ifndef VOXEL_GL_BUFFER_H
#define VOXEL_GL_BUFFER_H

#include <cstddef>
#include "GL/glew.h"
#include <vector>

template<typename T>
class GLBuffer {
public:
    virtual ~GLBuffer() = default;

protected:
    static constexpr size_t BASE_CAPACITY = 9;
    GLsizeiptr capacity = BASE_CAPACITY;

    GLuint bufferID {};

    bool isInit = false;

public:
    void free();

    virtual void write(const std::vector<T> &data) = 0;

    virtual void enable() = 0;

protected:
    void updateBufferCapacity(GLsizeiptr dataSize);
};

template<typename T>
class GLArrayBuffer : public GLBuffer<T> {
    GLuint bufferIndex {};
    GLint compCount {};

public:
    void init(GLuint index, GLint count);

    virtual void write(const std::vector<T> &data) override;

    virtual void enable() override;

    void disable();
};

class GLElementBuffer : public GLBuffer<unsigned short> {
    using elemType = unsigned short;

public:
    void init();

    virtual void write(const std::vector<elemType> &data) override;

    virtual void enable() override;
};

#endif //VOXEL_GL_BUFFER_H
