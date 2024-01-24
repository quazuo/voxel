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

OpenGLRenderer::OpenGLRenderer(int windowWidth, int windowHeight) : windowSize(windowWidth, windowHeight) {
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(windowWidth, windowHeight, "0x22's Voxel Engine", nullptr, nullptr);
    if (!window) {
        const char *desc;
        int code = glfwGetError(&desc);
        glfwTerminate();
        throw std::runtime_error("Failed to open GLFW window. Error: " + std::to_string(code) + " " + desc);
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(0);

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
    glfwSetCursorPos(window, 1024.0 / 2, 768.0 / 2);

    glfwPollEvents();

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

    // load & compile shaders
    cubeShaderID = loadShaders("cube-shader.vert", "cube-shader.frag");
    lineShaderID = loadShaders("line-shader.vert", "line-shader.frag");
    textShaderID = loadShaders("text-shader.vert", "text-shader.frag");
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
    lineVertices.init(3, 3);

    // generate buffers for hud text
    textVertices.init(0, 2);
    textUVs.init(1, 2);

    // init peripheral structures
    camera.init(window);
}

void OpenGLRenderer::tick(float deltaTime) {
    camera.tick(deltaTime);
    tickMouseMovement(deltaTime);
}

void OpenGLRenderer::tickMouseMovement(const float deltaTime) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    const float mouseSpeed = 0.05f;
    camera.updateRotation(
        mouseSpeed * deltaTime * (windowSize.x / 2 - (float) xpos),
        mouseSpeed * deltaTime * (windowSize.y / 2 - (float) ypos)
    );

    glfwSetCursorPos(window, (double) windowSize.x / 2, (double) windowSize.y / 2);
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

void OpenGLRenderer::loadTextures() const {
    textureManager->loadFontTexture("font.dds");
    textureManager->loadBlockTexture(EBlockType::BlockType_Grass, ALL_SIDE_FACES, "grass-side.dds");
    textureManager->loadBlockTexture(EBlockType::BlockType_Grass, EBlockFace::Top, "grass-top.dds");
    textureManager->loadBlockTexture(EBlockType::BlockType_Grass, EBlockFace::Bottom, "dirt.dds");
    textureManager->loadBlockTexture(EBlockType::BlockType_Dirt, ALL_FACES, "dirt.dds");
    textureManager->loadBlockTexture(EBlockType::BlockType_Stone, ALL_FACES, "stone.dds");
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    viewMatrix = glm::lookAt(camera.pos, camera.pos + camera.front, glm::vec3(0, 1, 0));
    projectionMatrix = glm::perspective(camera.fieldOfView, camera.aspectRatio, camera.zNear, camera.zFar);

    glm::vec3 lightPos = camera.pos;
    glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
}

void OpenGLRenderer::renderChunk(const std::shared_ptr<MeshContext> &ctx) {
    glUseProgram(cubeShaderID);
    textureManager->bindBlockTextures(cubeShaderID);

    // update buffers if needed
    if (ctx->isFreshlyUpdated) {
        ctx->writeToBuffers();
        ctx->isFreshlyUpdated = false;
    }

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), ctx->modelTranslate);
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;

    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvpMatrix[0][0]);
    glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(projectionMatrixID, 1, GL_FALSE, &projectionMatrix[0][0]);

    ctx->drawElements();
}

void OpenGLRenderer::renderOutline(const std::vector<glm::vec3> &vertices, const glm::mat4 &mvpMatrix,
                                   glm::vec3 color) {
    glUseProgram(lineShaderID);

    lineVertices.write(vertices);

    GLint mvpID = glGetUniformLocation(lineShaderID, "MVP");
    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &mvpMatrix[0][0]);

    GLint colorID = glGetUniformLocation(lineShaderID, "color");
    glUniform3f(colorID, color.r, color.g, color.b);

    lineVertices.enable();
    glDrawArrays(GL_LINES, 0, (GLsizei) vertices.size());
    lineVertices.disable();

    glUseProgram(cubeShaderID);
}

void OpenGLRenderer::renderCubeOutline(const glm::vec3 minVec, const float sideLength, const glm::vec3 color) {
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), minVec);
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;

    std::vector<glm::vec3> vertices;

    // add x-aligned lines
    std::vector<glm::vec3> vs = {
        {0, 0,          0},
        {0, sideLength, 0},
        {0, 0,          sideLength},
        {0, sideLength, sideLength}
    };
    for (const auto &v: vs) {
        vertices.push_back(v);
        vertices.push_back(v + glm::vec3(sideLength, 0, 0));
    }

    // add y-aligned lines
    vs = {
        {0,          0, 0},
        {sideLength, 0, 0},
        {0,          0, sideLength},
        {sideLength, 0, sideLength}
    };
    for (const auto &v: vs) {
        vertices.push_back(v);
        vertices.push_back(v + glm::vec3(0, sideLength, 0));
    }

    // add z-aligned lines
    vs = {
        {0,          0,          0},
        {sideLength, 0,          0},
        {0,          sideLength, 0},
        {sideLength, sideLength, 0}
    };
    for (const auto &v: vs) {
        vertices.push_back(v);
        vertices.push_back(v + glm::vec3(0, 0, sideLength));
    }

    renderOutline(vertices, mvpMatrix, color);
}

void OpenGLRenderer::renderChunkOutline(const glm::vec3 chunkPos, const glm::vec3 color) {
    const glm::vec3 chunkMinVec =
        chunkPos - glm::vec3(Block::RENDER_SIZE, Block::RENDER_SIZE, Block::RENDER_SIZE) * 0.5f;
    renderCubeOutline(chunkMinVec, Chunk::CHUNK_SIZE * Block::RENDER_SIZE, color);
}

void OpenGLRenderer::renderTargetedBlockOutline(const glm::vec3 blockPos) {
    const glm::vec3 minVec = blockPos - glm::vec3(Block::RENDER_SIZE, Block::RENDER_SIZE, Block::RENDER_SIZE) * 0.5f;
    renderCubeOutline(minVec, Block::RENDER_SIZE, {0, 1, 1});
}

void OpenGLRenderer::renderText(const std::string &text, float x, float y, size_t fontSize) {
    std::vector<glm::vec2> vertices;
    std::vector<glm::vec2> uvs;
    constexpr float widthMult = 0.7f;
    constexpr float uvOffset = 1.0f / 16 * (1.0f - widthMult) / 2;

    for (size_t i = 0; i < text.size(); i++) {
        glm::vec2 vertexUpLeft = glm::vec2(x + i * fontSize * widthMult, y + (float) fontSize);
        glm::vec2 vertexUpRight = glm::vec2(x + (i + 1) * fontSize * widthMult, y + (float) fontSize);
        glm::vec2 vertexDownRight = glm::vec2(x + (i + 1) * fontSize * widthMult, y);
        glm::vec2 vertexDownLeft = glm::vec2(x + i * fontSize * widthMult, y);

        vertices.push_back(vertexUpLeft);
        vertices.push_back(vertexDownLeft);
        vertices.push_back(vertexUpRight);

        vertices.push_back(vertexDownRight);
        vertices.push_back(vertexUpRight);
        vertices.push_back(vertexDownLeft);

        char character = text[i];
        glm::vec2 uv = {
            (float) (character % 16) / 16.0f,
            (float) (character / 16) / 16.0f
        };

        glm::vec2 uvUpLeft = glm::vec2(uv.x + uvOffset, uv.y);
        glm::vec2 uvUpRight = glm::vec2(uv.x + 1.0f / 16.0f - uvOffset, uv.y);
        glm::vec2 uvDownRight = glm::vec2(uv.x + 1.0f / 16.0f - uvOffset, uv.y + 1.0f / 16.0f);
        glm::vec2 uvDownLeft = glm::vec2(uv.x + uvOffset, uv.y + 1.0f / 16.0f);

        uvs.push_back(uvUpLeft);
        uvs.push_back(uvDownLeft);
        uvs.push_back(uvUpRight);

        uvs.push_back(uvDownRight);
        uvs.push_back(uvUpRight);
        uvs.push_back(uvDownLeft);
    }

    glUseProgram(textShaderID);
    textureManager->bindFontTexture(textShaderID);

    textVertices.write(vertices);
    textUVs.write(uvs);

    textVertices.enable();
    textUVs.enable();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_DEPTH_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) vertices.size());

    glDisable(GL_BLEND);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glUseProgram(cubeShaderID);
}

void OpenGLRenderer::renderHud() {
    constexpr float crosshairLength = 0.02;
    std::vector<glm::vec3> vertices;

    constexpr glm::vec3 left = {-crosshairLength, 0, 0};
    constexpr glm::vec3 right = {crosshairLength, 0, 0};
    constexpr glm::vec3 top = {0, -crosshairLength, 0};
    constexpr glm::vec3 bottom = {0, crosshairLength, 0};

    vertices.push_back(left);
    vertices.push_back(right);

    vertices.push_back(top * windowSize.x / windowSize.y);
    vertices.push_back(bottom * windowSize.x / windowSize.y);

    glClear(GL_DEPTH_BUFFER_BIT);
    renderOutline(vertices, glm::mat4(1), {1, 1, 1});
}

void OpenGLRenderer::finishRendering() {
    glFlush();
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                   const GLchar *message, const void *userParam) {
    (void) userParam;
    (void) length;

    std::stringstream ss;
    ss << "ERROR!\nsource: ";

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            ss << "GL_DEBUG_SOURCE_API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            ss << "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            ss << "GL_DEBUG_SOURCE_SHADER_COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            ss << "GL_DEBUG_SOURCE_THIRD_PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            ss << "GL_DEBUG_SOURCE_APPLICATION";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            ss << "GL_DEBUG_SOURCE_OTHER";
            break;
        default:
            break;
    }

    ss << "\ntype:";

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            ss << "GL_DEBUG_TYPE_ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            ss << "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            ss << "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            ss << "GL_DEBUG_TYPE_PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            ss << "GL_DEBUG_TYPE_PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_MARKER:
            ss << "GL_DEBUG_TYPE_MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            ss << "GL_DEBUG_TYPE_PUSH_GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            ss << "GL_DEBUG_TYPE_POP_GROUP";
            break;
        case GL_DEBUG_TYPE_OTHER:
            ss << "GL_DEBUG_TYPE_OTHER";
            break;
        default:
            break;
    }

    ss << "\nid: " << id;
    ss << "\nseverity:";

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            ss << "GL_DEBUG_SEVERITY_HIGH";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            ss << "GL_DEBUG_SEVERITY_MEDIUM";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            ss << "GL_DEBUG_SEVERITY_LOW";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            ss << "GL_DEBUG_SEVERITY_NOTIFICATION";
            break;
        default:
            break;
    }

    ss << "\nmessage: " << message << "\n";

    throw std::runtime_error(ss.str());
}

void OpenGLRenderer::terminate() {
    glfwTerminate();
}
