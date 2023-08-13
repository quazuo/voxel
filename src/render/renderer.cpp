#include "renderer.h"
#include "src/utils/vec.h"
#include "mesh-context.h"

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, nullptr);

    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    // Create and compile our GLSL program from the shaders
    cubeShaderID = loadShaders("cube-shader.vert", "cube-shader.frag");
    lineShaderID = loadShaders("line-shader.vert", "line-shader.frag");
    glUseProgram(cubeShaderID);

    // Get a handle for our "MVP" uniform
    mvpMatrixID = glGetUniformLocation(cubeShaderID, "MVP");
    modelMatrixID = glGetUniformLocation(cubeShaderID, "M");
    viewMatrixID = glGetUniformLocation(cubeShaderID, "V");
    projectionMatrixID = glGetUniformLocation(cubeShaderID, "P");

    loadTextures();

    // Get a handle for our "LightPosition" uniform
    lightID = glGetUniformLocation(cubeShaderID, "LightPosition_worldspace");

    // generate buffer for line vertices and allocate it beforehand
    glGenBuffers(1, &lineVertexArrayID);
    glBindBuffer(GL_ARRAY_BUFFER, lineVertexArrayID);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    isInit = true;
}

void OpenGLRenderer::tick(float deltaTime) {
    camera.tick(window, deltaTime);
}

GLuint OpenGLRenderer::loadShaders(const std::filesystem::path &vertexShaderPath,
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
        throw std::runtime_error("vertex shader compilation failed: " + std::string(&vertexShaderErrorMessage[0]));
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
        throw std::runtime_error("fragment shader compilation failed: " + std::string(&fragmentShaderErrorMessage[0]));
    }

    // Link the program
    std::cout << "Linking program\n";
    GLuint shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShaderID);
    glAttachShader(shaderID, fragmentShaderID);
    glLinkProgram(shaderID);

    // Check the program
    glGetProgramiv(shaderID, GL_LINK_STATUS, &Result);
    glGetProgramiv(shaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(shaderID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
        throw std::runtime_error("shader linking failed: " + std::string(&ProgramErrorMessage[0]));
    }

    glDetachShader(shaderID, vertexShaderID);
    glDetachShader(shaderID, fragmentShaderID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return shaderID;
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    viewMatrix = glm::lookAt(camera.pos, camera.pos + camera.front, glm::vec3(0, 1, 0));
    projectionMatrix = glm::perspective(camera.fieldOfView, camera.aspectRatio, camera.zNear, camera.zFar);

    glm::vec3 lightPos = camera.pos;
    glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
}

void OpenGLRenderer::renderChunk(const MeshContext &ctx) {
    const IndexedMeshData &indexedData = ctx.getIndexedData();

    glUseProgram(cubeShaderID);

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

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), ctx.modelTranslate);
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;

    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvpMatrix[0][0]);
    glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(projectionMatrixID, 1, GL_FALSE, &projectionMatrix[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Vertex));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::UV));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Normal));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx.getBufferID(MeshContext::EBufferType::Element));
    glDrawElements(GL_TRIANGLES, indexedData.indices.size(), GL_UNSIGNED_SHORT, nullptr);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

void OpenGLRenderer::renderOutline(const std::vector<glm::vec3> &vertices, const glm::mat4 &mvpMatrix,
                                   glm::vec3 color) const {
    glUseProgram(lineShaderID);

    glBindBuffer(GL_ARRAY_BUFFER, lineVertexArrayID);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvpMatrix[0][0]);

    GLuint colorID = glGetUniformLocation(lineShaderID, "color");
    glUniform3f(colorID, color.r, color.g, color.b);

    glEnableVertexAttribArray(3);

    glDrawArrays(GL_LINES, 0, vertices.size());

    glDisableVertexAttribArray(3);

    glUseProgram(cubeShaderID);
}

void OpenGLRenderer::renderChunkOutline(const glm::vec3 chunkPos, glm::vec3 color) const {
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), chunkPos);
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;

    const glm::vec3 offset = {-Block::RENDER_SIZE / 2, -Block::RENDER_SIZE / 2, -Block::RENDER_SIZE / 2};
    const float len = Chunk::CHUNK_SIZE * Block::RENDER_SIZE;

    std::vector<glm::vec3> vertices;

    // add x-aligned lines
    std::vector<glm::vec3> vs = {
        {0, 0,   0},
        {0, len, 0},
        {0, 0,   len},
        {0, len, len}
    };
    for (const auto &v: vs) {
        vertices.push_back(v + offset);
        vertices.push_back(v + glm::vec3(len, 0, 0) + offset);
    }

    // add y-aligned lines
    vs = {
        {0,   0, 0},
        {len, 0, 0},
        {0,   0, len},
        {len, 0, len}
    };
    for (const auto &v: vs) {
        vertices.push_back(v + offset);
        vertices.push_back(v + glm::vec3(0, len, 0) + offset);
    }

    // add z-aligned lines
    vs = {
        {0,   0,   0},
        {len, 0,   0},
        {0,   len, 0},
        {len, len, 0}
    };
    for (const auto &v: vs) {
        vertices.push_back(v + offset);
        vertices.push_back(v + glm::vec3(0, 0, len) + offset);
    }

    renderOutline(vertices, mvpMatrix, color);
}

void OpenGLRenderer::renderFrustumOutline() const {
    static Camera cam;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        cam = camera;
    }

    const float halfVSideFar = camera.zFar * tanf(camera.fieldOfView * 0.5f);
    const float halfHSideFar = halfVSideFar * camera.aspectRatio;

    const float halfVSideNear = cam.zNear * tanf(cam.fieldOfView * 0.5f);
    const float halfHSideNear = halfVSideNear * cam.aspectRatio;

    const glm::vec3 frontMultNear = cam.zNear * cam.front;
    const glm::vec3 frontMultFar = cam.zFar * cam.front;

    const glm::vec3 nearPlaneVertices[4] = {
        cam.pos + frontMultNear + cam.up * halfVSideNear + cam.right * halfHSideNear,
        cam.pos + frontMultNear - cam.up * halfVSideNear + cam.right * halfHSideNear,
        cam.pos + frontMultNear - cam.up * halfVSideNear - cam.right * halfHSideNear,
        cam.pos + frontMultNear + cam.up * halfVSideNear - cam.right * halfHSideNear,
    };

    const glm::vec3 farPlaneVertices[4] = {
        cam.pos + frontMultFar + cam.up * halfVSideFar + cam.right * halfHSideFar,
        cam.pos + frontMultFar - cam.up * halfVSideFar + cam.right * halfHSideFar,
        cam.pos + frontMultFar - cam.up * halfVSideFar - cam.right * halfHSideFar,
        cam.pos + frontMultFar + cam.up * halfVSideFar - cam.right * halfHSideFar,
    };

    std::vector<glm::vec3> vertices;
    for (int i = 0; i < 4; i++) {
        // near plane
        vertices.push_back(nearPlaneVertices[i]);
        vertices.push_back(nearPlaneVertices[(i + 1) % 4]);

        // far plane
        vertices.push_back(farPlaneVertices[i]);
        vertices.push_back(farPlaneVertices[(i + 1) % 4]);

        // the rest
        vertices.push_back(nearPlaneVertices[i]);
        vertices.push_back(farPlaneVertices[i]);
    }

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
    const glm::vec3 color = {1, 1, 1};
    renderOutline(vertices, mvpMatrix, color);
}

void OpenGLRenderer::finishRendering() {
    // renderFrustumOutline();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::loadTextures() const {
    texManager->loadTexture(EBlockType::BlockType_Grass, "grass.dds");
    texManager->loadTexture(EBlockType::BlockType_Dirt, "dirt.dds");
    texManager->bindTextures(cubeShaderID);
}

bool OpenGLRenderer::isChunkInFrustum(const Chunk &chunk) const {
    return camera.isChunkInFrustum(chunk.getPos());
}

void OpenGLRenderer::debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                   const GLchar *message, const void *userParam) {
    (void) userParam;
    (void) length;

    std::cout << "ERROR!\nsource: ";

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            std::cout << "GL_DEBUG_SOURCE_API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            std::cout << "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            std::cout << "GL_DEBUG_SOURCE_SHADER_COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            std::cout << "GL_DEBUG_SOURCE_THIRD_PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            std::cout << "GL_DEBUG_SOURCE_APPLICATION";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            std::cout << "GL_DEBUG_SOURCE_OTHER";
            break;
        default:
            break;
    }

    std::cout << "\ntype:";

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            std::cout << "GL_DEBUG_TYPE_ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            std::cout << "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            std::cout << "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            std::cout << "GL_DEBUG_TYPE_PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            std::cout << "GL_DEBUG_TYPE_PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_MARKER:
            std::cout << "GL_DEBUG_TYPE_MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            std::cout << "GL_DEBUG_TYPE_PUSH_GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            std::cout << "GL_DEBUG_TYPE_POP_GROUP";
            break;
        case GL_DEBUG_TYPE_OTHER:
            std::cout << "GL_DEBUG_TYPE_OTHER";
            break;
        default:
            break;
    }

    std::cout << "\nid: " << id;
    std::cout << "\nseverity:";

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            std::cout << "GL_DEBUG_SEVERITY_HIGH";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cout << "GL_DEBUG_SEVERITY_MEDIUM";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            std::cout << "GL_DEBUG_SEVERITY_LOW";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cout << "GL_DEBUG_SEVERITY_NOTIFICATION";
            break;
        default:
            break;
    }

    std::cout << "\nmessage: " << message << "\n\n";
}
