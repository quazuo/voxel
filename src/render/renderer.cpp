#include "renderer.h"

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * MeshContext
 */

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
        data.vertices.push_back(vertex.position.toGlm());
        data.uvs.push_back(vertex.uv.toGlm());
        data.normals.push_back(vertex.normal.toGlm());
        unsigned short newIndex = (unsigned short) data.vertices.size() - 1;
        data.indices.push_back(newIndex);
        vertexToOutIndex[vertex] = newIndex;
    }
}

/**
 * IndexedMeshData
 */

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
    glGenBuffers(1, &vertexBufferID);
    glGenBuffers(1, &uvBufferID);
    glGenBuffers(1, &normalBufferID);
    glGenBuffers(1, &elementBufferID);

    // 1st attribute buffer: vertices
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glVertexAttribPointer(
        0,              // attribute
        3,              // size
        GL_FLOAT,       // type
        GL_FALSE,       // normalized?
        0,              // stride
        nullptr         // array buffer offset
    );

    // 2nd attribute buffer: UVs
    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // 3rd attribute buffer: normals
    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // initialize the buffers
    size_t bufferSize = USHRT_MAX / 2;

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(glm::vec3), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(glm::vec2), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(glm::vec3), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize * sizeof(unsigned short), nullptr, GL_STATIC_DRAW);
}

GLuint MeshContext::getBufferID(MeshContext::EBufferType bufferType) const {
    switch (bufferType) {
        case EBufferType::Vertex:
            return vertexBufferID;
        case EBufferType::UV:
            return uvBufferID;
        case EBufferType::Normal:
            return normalBufferID;
        case EBufferType::Element:
        default:
            return elementBufferID;
    }
}

void MeshContext::freeBuffers() {
    const size_t nBuffers = 4;
    const GLuint buffers[nBuffers] = {vertexBufferID, uvBufferID, normalBufferID, elementBufferID};
    glDeleteBuffers(nBuffers, buffers);
}

/**
 * OpenGLRenderer
 */

void OpenGLRenderer::init() {
    if (isInit) return;

    // Initialise GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1024, 768, "0x22's Voxel Engine", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to open GLFW window. "
                                 "If you have an Intel GPU, they are not 3.3 compatible. "
                                 "Try the 2.1 version of the tutorials.");
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024 / 2, 768 / 2);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    // Create and compile our GLSL program from the shaders
    std::filesystem::path vertexShaderPath = "shader.vert";
    std::filesystem::path fragmentShaderPath = "shader.frag";
    loadShaders(vertexShaderPath, fragmentShaderPath);

    // Get a handle for our "MVP" uniform
    mvpMatrixID = glGetUniformLocation(programID, "MVP");
    modelMatrixID = glGetUniformLocation(programID, "M");
    viewMatrixID = glGetUniformLocation(programID, "V");
    projectionMatrixID = glGetUniformLocation(programID, "P");

    loadTextureDDS("grass.dds");

    // Get a handle for our "textureSampler" uniform
    textureSamplerID = glGetUniformLocation(programID, "textureSampler");

    // Get a handle for our "LightPosition" uniform
    glUseProgram(programID);
    lightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    isInit = true;
}

void OpenGLRenderer::tick(float deltaTime) {
    tickUserInputs(deltaTime);
}

void OpenGLRenderer::loadShaders(const std::filesystem::path &vertexShaderPath,
                                 const std::filesystem::path &fragmentShaderPath) {
    // Create the shaders
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the vertex Shader code from the file
    std::ifstream vertexShaderStream(vertexShaderPath, std::ios::in);
    if (!vertexShaderStream.is_open()) {
        throw std::runtime_error("Impossible to open vertex shader file");
    }

    std::stringstream sstr;
    sstr << vertexShaderStream.rdbuf();
    std::string vertexShaderCode = sstr.str();
    vertexShaderStream.close();

    // Read the fragment Shader code from the file
    std::ifstream fragmentShaderStream(fragmentShaderPath, std::ios::in);
    if (!fragmentShaderStream.is_open()) {
        throw std::runtime_error("Impossible to open fragment shader file");
    }

    sstr.clear();
    sstr.str(std::string());
    sstr << fragmentShaderStream.rdbuf();
    std::string fragmentShaderCode = sstr.str();
    fragmentShaderStream.close();

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile vertex shader
    std::cout << "Compiling shader: " << vertexShaderPath << "\n";
    char const *vertexSourcePointer = vertexShaderCode.c_str();
    glShaderSource(vertexShaderID, 1, &vertexSourcePointer, nullptr);
    glCompileShader(vertexShaderID);

    // Check vertex shader
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> vertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(vertexShaderID, InfoLogLength, nullptr, &vertexShaderErrorMessage[0]);
        printf("%s\n", &vertexShaderErrorMessage[0]);
    }

    // Compile fragment shader
    std::cout << "Compiling shader: " << fragmentShaderPath << "\n";
    char const *fragmentSourcePointer = fragmentShaderCode.c_str();
    glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer, nullptr);
    glCompileShader(fragmentShaderID);

    // Check fragment shader
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> fragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(fragmentShaderID, InfoLogLength, nullptr, &fragmentShaderErrorMessage[0]);
        printf("%s\n", &fragmentShaderErrorMessage[0]);
    }

    // Link the program
    std::cout << "Linking program\n";
    programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    // Check the program
    glGetProgramiv(programID, GL_LINK_STATUS, &Result);
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(programID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(programID, vertexShaderID);
    glDetachShader(programID, fragmentShaderID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
}

void OpenGLRenderer::loadTextureDDS(const std::filesystem::path &path) {
    std::ifstream fileStream(path, std::ios::in);
    if (!fileStream.is_open()) {
        throw std::runtime_error(path.string() + " could not be opened.\n");
    }

    // verify the type of file
    constexpr size_t fileCodeSize = 4;
    char filecode[fileCodeSize];
    fileStream.read(filecode, fileCodeSize);
    if (strncmp(filecode, "DDS ", 4) != 0) {
        fileStream.close();
        throw std::runtime_error(path.string() + "is not a valid DDS file\n");
        return;
    }

    // get the surface desc
    constexpr size_t headerSize = 124;
    char header[headerSize];
    fileStream.read(header, headerSize);

    uint32_t height = *(uint32_t *) &(header[8]);
    uint32_t width = *(uint32_t *) &(header[12]);
    uint32_t linearSize = *(uint32_t *) &(header[16]);
    uint32_t mipMapCount = *(uint32_t *) &(header[24]);
    uint32_t fourCC = *(uint32_t *) &(header[80]);

    unsigned int bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
    std::vector<char> buffer(bufsize);
    fileStream.read(&buffer[0], bufsize);

    fileStream.close();

    constexpr uint32_t FOURCC_DXT1 = 0x31545844;
    constexpr uint32_t FOURCC_DXT3 = 0x33545844;
    constexpr uint32_t FOURCC_DXT5 = 0x35545844;

    // unsigned int components = (fourCC == FOURCC_DXT1) ? 3 : 4;
    uint32_t format;
    switch (fourCC) {
        case FOURCC_DXT1:
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case FOURCC_DXT3:
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case FOURCC_DXT5:
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        default:
            return;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    // load the mipmaps
    for (unsigned int level = 0; level < mipMapCount && (width || height); ++level) {
        unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
        glCompressedTexImage2D(
            GL_TEXTURE_2D,
            (GLint) level,
            format,
            (GLsizei) width,
            (GLsizei) height,
            0,
            (GLsizei) size,
            buffer.begin().base() + offset
        );

        offset += size;
        width /= 2;
        height /= 2;

        // deal with Non-Power-Of-Two textures
        if (width < 1) width = 1;
        if (height < 1) height = 1;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use previously compiled shaders
    glUseProgram(programID);

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(textureSamplerID, 0);

    glm::vec3 lightPos = cameraPos.toGlm();
    glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
}

void OpenGLRenderer::renderMesh(const MeshContext &ctx) {
    const IndexedMeshData &indexedData = ctx.getIndexedData();

    // make buffers
    if (ctx.isFreshlyUpdated) {
        glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Vertex));
        glBufferSubData(GL_ARRAY_BUFFER, 0, indexedData.vertices.size() * sizeof(glm::vec3),
                        &indexedData.vertices[0]);

        glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::UV));
        glBufferSubData(GL_ARRAY_BUFFER, 0, indexedData.uvs.size() * sizeof(glm::vec2),
                        &indexedData.uvs[0]);

        glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Normal));
        glBufferSubData(GL_ARRAY_BUFFER, 0, indexedData.normals.size() * sizeof(glm::vec3),
                        &indexedData.normals[0]);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Element));
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexedData.indices.size() * sizeof(unsigned short),
                        &indexedData.indices[0]);
    }

    glm::vec3 direction(
        cos(cameraRot.y) * sin(cameraRot.x),
        sin(cameraRot.y),
        cos(cameraRot.y) * cos(cameraRot.x)
    );

    // compute MVP matrix and its components
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), ctx.modelTranslate.toGlm());
    glm::mat4 viewMatrix = glm::lookAt(
        cameraPos.toGlm(),              // camera's position
        cameraPos.toGlm() + direction,  // the point at which the camera is looking
        glm::vec3(0, 1, 0)              // head vector
    );
    glm::mat4 projectionMatrix = glm::perspective(
        glm::radians(80.0f),    // field of view
        4.0f / 3.0f,            // aspect ratio
        0.1f,                   // near clipping plane
        100.0f                  // far clipping plane
    );
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;

    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvpMatrix[0][0]);
    glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(projectionMatrixID, 1, GL_FALSE, &projectionMatrix[0][0]);

    // 1st attribute buffer: vertices
    glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Vertex));
    glVertexAttribPointer(
        0,              // attribute
        3,              // size
        GL_FLOAT,       // type
        GL_FALSE,       // normalized?
        0,              // stride
        nullptr         // array buffer offset
    );

    // 2nd attribute buffer: UVs
    glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::UV));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // 3rd attribute buffer: normals
    glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Normal));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Element));

    // draw the triangles
    glDrawElements(
        GL_TRIANGLES,                       // mode
        indexedData.indices.size(),         // count
        GL_UNSIGNED_SHORT,                  // type
        nullptr                             // element array buffer offset
    );

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

void OpenGLRenderer::finishRendering() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::tickUserInputs(float deltaTime) {
    const float rotationSpeed = 2.5;
    const float movementSpeed = 8.0;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        cameraRot.y = std::clamp(
            cameraRot.y + (double) (deltaTime * rotationSpeed),
            -3.14 / 2,
            3.14 / 2
        );
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        cameraRot.y = std::clamp(
            cameraRot.y - (double) (deltaTime * rotationSpeed),
            -3.14 / 2,
            3.14 / 2
        );
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraRot.x -= (double) (deltaTime * rotationSpeed);
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        cameraRot.x += (double) (deltaTime * rotationSpeed);
    }

    glm::vec3 direction(
        cos(cameraRot.y) * sin(cameraRot.x),
        sin(cameraRot.y),
        cos(cameraRot.y) * cos(cameraRot.x)
    );

    glm::vec3 right = glm::vec3(
        sin(cameraRot.x - 3.14 / 2.0),
        0,
        cos(cameraRot.x - 3.14 / 2.0)
    );

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cameraPos += Vec3::fromGlm(direction * deltaTime * movementSpeed); // Move forward
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraPos -= Vec3::fromGlm(direction * deltaTime * movementSpeed); // Move backward
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cameraPos += Vec3::fromGlm(right * deltaTime * movementSpeed); // Strafe right
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cameraPos -= Vec3::fromGlm(right * deltaTime * movementSpeed); // Strafe left
    }
}
