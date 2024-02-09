#include "gl-shader.h"

#include <fstream>
#include <iostream>

GLShader::GLShader(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath) {
    // create the shaders
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // read the vertex shader code from the file
    std::ifstream vertexShaderStream(vertexShaderPath, std::ios::in);
    if (!vertexShaderStream.is_open()) {
        throw std::runtime_error("Impossible to open vertex shader file");
    }

    std::stringstream sstr;
    sstr << vertexShaderStream.rdbuf();
    std::string vertexShaderCode = sstr.str();
    vertexShaderStream.close();

    // read the fragment shader code from the file
    std::ifstream fragmentShaderStream(fragmentShaderPath, std::ios::in);
    if (!fragmentShaderStream.is_open()) {
        throw std::runtime_error("Impossible to open fragment shader file");
    }

    sstr.clear();
    sstr.str(std::string());
    sstr << fragmentShaderStream.rdbuf();
    std::string fragmentShaderCode = sstr.str();
    fragmentShaderStream.close();

    GLint result = GL_FALSE;
    int infoLogLength;

    // compile the vertex shader
    std::cout << "Compiling shader: " << vertexShaderPath << "\n";
    char const *vertexSourcePointer = vertexShaderCode.c_str();
    glShaderSource(vertexShaderID, 1, &vertexSourcePointer, nullptr);
    glCompileShader(vertexShaderID);

    // check the vertex shader
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> vertexShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(vertexShaderID, infoLogLength, nullptr, &vertexShaderErrorMessage[0]);
        throw std::runtime_error("vertex shader compilation failed: " + std::string(&vertexShaderErrorMessage[0]));
    }

    // compile the fragment shader
    std::cout << "Compiling shader: " << fragmentShaderPath << "\n";
    char const *fragmentSourcePointer = fragmentShaderCode.c_str();
    glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer, nullptr);
    glCompileShader(fragmentShaderID);

    // check the fragment shader
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> fragmentShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(fragmentShaderID, infoLogLength, nullptr, &fragmentShaderErrorMessage[0]);
        throw std::runtime_error("fragment shader compilation failed: " + std::string(&fragmentShaderErrorMessage[0]));
    }

    // link the program
    std::cout << "Linking program\n";
    shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShaderID);
    glAttachShader(shaderID, fragmentShaderID);
    glLinkProgram(shaderID);

    // check the program
    glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
    glGetProgramiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(infoLogLength + 1);
        glGetProgramInfoLog(shaderID, infoLogLength, nullptr, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
        throw std::runtime_error("shader linking failed: " + std::string(&ProgramErrorMessage[0]));
    }

    glDetachShader(shaderID, vertexShaderID);
    glDetachShader(shaderID, fragmentShaderID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
}

void GLShader::enable() const {
    glUseProgram(shaderID);
}

void GLShader::setUniform(const std::string &name, const GLint value) {
    const GLuint uniformID = getUniformID(name);
    glUniform1i(uniformID, value);
}

void GLShader::setUniform(const std::string &name, const std::vector<GLint> &value) {
    const GLuint uniformID = getUniformID(name);
    glUniform1iv(
            uniformID,
            static_cast<GLint>(value.size()),
            value.data()
    );
}

void GLShader::setUniform(const std::string &name, const glm::vec3 &value) {
    const GLuint uniformID = getUniformID(name);
    glUniform3f(uniformID, value.x, value.y, value.z);
}

void GLShader::setUniform(const std::string &name, const glm::mat4 &value) {
    const GLuint uniformID = getUniformID(name);
    glUniformMatrix4fv(uniformID, 1, GL_FALSE, &value[0][0]);
}

GLuint GLShader::getUniformID(const std::string &name) {
    if (uniformIDs.contains(name)) {
        return uniformIDs.at(name);
    }

    const auto id = glGetUniformLocation(shaderID, name.c_str());
    if (id == -1) {
        throw std::runtime_error("failed to get uniform with name: " + name);
    }

    uniformIDs.emplace(name, id);
    return id;
}

