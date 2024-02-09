#ifndef SHADER_H
#define SHADER_H

#include <filesystem>
#include <unordered_map>

#include <GL/glew.h>
#include <glm/glm.hpp>

class GLShader {
    GLuint shaderID;

    std::unordered_map<std::string, GLuint> uniformIDs {};

public:
    GLShader(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

    [[nodiscard]]
    GLuint getID() const { return shaderID; }

    void enable() const;

    void setUniform(const std::string& name, GLint value);

    void setUniform(const std::string& name, const std::vector<GLint>& value);

    void setUniform(const std::string& name, const glm::vec3& value);

    void setUniform(const std::string& name, const glm::mat4& value);

private:
    [[nodiscard]]
    GLuint getUniformID(const std::string& name);
};

#endif //SHADER_H
