#include "renderer.h"
#include "mesh-context.h"

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include <ranges>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// vertices of a the skybox cube.
// might change this to be generated more intelligently... but it's good enough for now
static std::vector<glm::vec3> skyboxVerticesList = {
    {-1.0f, 1.0f, -1.0f},
    {-1.0f, -1.0f, -1.0f},
    {1.0f, -1.0f, -1.0f},
    {1.0f, -1.0f, -1.0f},
    {1.0f, 1.0f, -1.0f},
    {-1.0f, 1.0f, -1.0f},

    {-1.0f, -1.0f, 1.0f},
    {-1.0f, -1.0f, -1.0f},
    {-1.0f, 1.0f, -1.0f},
    {-1.0f, 1.0f, -1.0f},
    {-1.0f, 1.0f, 1.0f},
    {-1.0f, -1.0f, 1.0f},

    {1.0f, -1.0f, -1.0f},
    {1.0f, -1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, -1.0f},
    {1.0f, -1.0f, -1.0f},

    {-1.0f, -1.0f, 1.0f},
    {-1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f},
    {-1.0f, -1.0f, 1.0f},

    {-1.0f, 1.0f, -1.0f},
    {1.0f, 1.0f, -1.0f},
    {1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, -1.0f},

    {-1.0f, -1.0f, -1.0f},
    {-1.0f, -1.0f, 1.0f},
    {1.0f, -1.0f, -1.0f},
    {1.0f, -1.0f, -1.0f},
    {-1.0f, -1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f}
};

OpenGLRenderer::OpenGLRenderer(const int windowWidth, const int windowHeight) {
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(windowWidth, windowHeight, "0x22's Voxel Engine", nullptr, nullptr);
    if (!window) {
        const char *desc;
        const int code = glfwGetError(&desc);
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
    glfwSetCursorPos(window, static_cast<double>(windowWidth) / 2, static_cast<double>(windowHeight) / 2);

    glfwPollEvents();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // load & compile shaders
    cubeShaderID = loadShaders("shaders/cube-shader.vert", "shaders/cube-shader.frag");
    skyboxShaderID = loadShaders("shaders/skybox-shader.vert", "shaders/skybox-shader.frag");
    lineShaderID = loadShaders("shaders/line-shader.vert", "shaders/line-shader.frag");
    glUseProgram(cubeShaderID);

    // Get a handle for our "MVP" uniform
    mvpMatrixID = glGetUniformLocation(cubeShaderID, "MVP");
    modelMatrixID = glGetUniformLocation(cubeShaderID, "M");
    viewMatrixID = glGetUniformLocation(cubeShaderID, "V");
    projectionMatrixID = glGetUniformLocation(cubeShaderID, "P");

    loadTextures();

    skyboxVao = std::make_unique<BasicVertexArray>();
    skyboxVao->writeToBuffers(skyboxVerticesList);

    outlinesVao = std::make_unique<BasicVertexArray>();

    // init peripheral structures
    camera = std::make_unique<Camera>(window);
}

OpenGLRenderer::~OpenGLRenderer() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::tick(const float deltaTime) const {
    camera->tick(deltaTime);
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
    GLuint shaderID = glCreateProgram();
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

    return shaderID;
}

void OpenGLRenderer::loadTextures() const {
    using FacePathMapping = FaceMapping<std::filesystem::path>;

    const std::map<EBlockType, FacePathMapping> blockTexturePathMappings{
        {
            BlockType_Grass, FacePathMapping(
                std::make_pair(ALL_SIDE_FACES, "assets/grass-side.png"),
                std::make_pair(Top, "assets/grass-top.png"),
                std::make_pair(Bottom, "assets/dirt.png")
            )
        },
        {
            BlockType_Dirt, FacePathMapping(
                std::make_pair(ALL_FACES, "assets/dirt.png")
            )
        },
        {
            BlockType_Stone, FacePathMapping(
                std::make_pair(ALL_FACES, "assets/stone.png")
            )
        }
    };

    const FacePathMapping skyboxTexturePaths{
        std::make_pair(ALL_SIDE_FACES, "assets/sky-side.png"),
        std::make_pair(Top, "assets/sky-top.png"),
        std::make_pair(Bottom, "assets/sky-bottom.png")
    };

    textureManager->loadBlockTextures(blockTexturePathMappings);
    textureManager->loadSkyboxTextures(skyboxTexturePaths);
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    viewMatrix = camera->getViewMatrix();
    projectionMatrix = camera->getProjectionMatrix();

    for (auto &vertices: tempLineVertexGroups | std::views::values) {
        vertices.clear();
    }
}

void OpenGLRenderer::setIsCursorLocked(const bool b) const {
    camera->setIsCursorLocked(b);
    glfwSetInputMode(window, GLFW_CURSOR, b ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void OpenGLRenderer::renderGuiSection() const {
    camera->renderGuiSection();
}

void OpenGLRenderer::renderSkybox() {
    glDepthMask(GL_FALSE);
    glUseProgram(skyboxShaderID);

    const glm::mat4 staticViewMatrix = camera->getStaticViewMatrix();
    const auto viewMatrixID = glGetUniformLocation(skyboxShaderID, "V");
    glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &staticViewMatrix[0][0]);

    const auto projectionMatrixID = glGetUniformLocation(skyboxShaderID, "P");
    glUniformMatrix4fv(projectionMatrixID, 1, GL_FALSE, &projectionMatrix[0][0]);

    textureManager->bindSkyboxTextures(skyboxShaderID);

    skyboxVao->enable();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
}

void OpenGLRenderer::renderChunk(const std::shared_ptr<ChunkMeshContext> &ctx) {
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

void OpenGLRenderer::renderOutlines() {
    constexpr auto modelMatrix = glm::mat4(1.0f);
    const glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;

    glUseProgram(lineShaderID);

    for (const auto &[gid, vertices]: tempLineVertexGroups) {
        outlinesVao->writeToBuffers(vertices);

        const GLint mvpID = glGetUniformLocation(lineShaderID, "MVP");
        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &mvpMatrix[0][0]);

        const GLint colorID = glGetUniformLocation(lineShaderID, "color");
        const glm::vec3 color = vertexGroupColors.at(gid);
        glUniform3f(colorID, color.r, color.g, color.b);

        outlinesVao->enable();
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    }

    glUseProgram(cubeShaderID);
}

void OpenGLRenderer::addCubeOutline(const glm::vec3 &minVec, const float sideLength, const LineType gid) {
    std::vector<glm::vec3> &vertexGroup = tempLineVertexGroups[gid];

    // add x-aligned lines
    std::vector<glm::vec3> vs = {
        {0, 0, 0},
        {0, sideLength, 0},
        {0, 0, sideLength},
        {0, sideLength, sideLength}
    };
    for (const auto &v: vs) {
        vertexGroup.push_back(minVec + v);
        vertexGroup.push_back(minVec + v + glm::vec3(sideLength, 0, 0));
    }

    // add y-aligned lines
    vs = {
        {0, 0, 0},
        {sideLength, 0, 0},
        {0, 0, sideLength},
        {sideLength, 0, sideLength}
    };
    for (const auto &v: vs) {
        vertexGroup.push_back(minVec + v);
        vertexGroup.push_back(minVec + v + glm::vec3(0, sideLength, 0));
    }

    // add z-aligned lines
    vs = {
        {0, 0, 0},
        {sideLength, 0, 0},
        {0, sideLength, 0},
        {sideLength, sideLength, 0}
    };
    for (const auto &v: vs) {
        vertexGroup.push_back(minVec + v);
        vertexGroup.push_back(minVec + v + glm::vec3(0, 0, sideLength));
    }
}

void OpenGLRenderer::addChunkOutline(const glm::vec3 &chunkPos, const LineType gid) {
    const glm::vec3 chunkMinVec = chunkPos - Block::RENDER_SIZE / 2;
    addCubeOutline(chunkMinVec, Chunk::CHUNK_SIZE * Block::RENDER_SIZE, gid);
}

auto OpenGLRenderer::addTargetedBlockOutline(const glm::vec3 &blockPos) -> void {
    const glm::vec3 minVec = blockPos - Block::RENDER_SIZE / 2;
    addCubeOutline(minVec, Block::RENDER_SIZE, SELECTED_BLOCK_OUTLINE);
}

void OpenGLRenderer::renderHud() const {
    constexpr float crosshairLength = 0.02;
    std::vector<glm::vec3> vertices;

    constexpr glm::vec3 left = {-crosshairLength, 0, 0};
    constexpr glm::vec3 right = {crosshairLength, 0, 0};
    constexpr glm::vec3 top = {0, -crosshairLength, 0};
    constexpr glm::vec3 bottom = {0, crosshairLength, 0};

    vertices.push_back(left);
    vertices.push_back(right);

    glm::vec<2, int> windowSize{};
    glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

    vertices.push_back(top * static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y));
    vertices.push_back(bottom * static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y));

    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(lineShaderID);

    outlinesVao->writeToBuffers(vertices);

    constexpr auto mvpMatrix = glm::mat4(1.0f);
    const GLint mvpID = glGetUniformLocation(lineShaderID, "MVP");
    glUniformMatrix4fv(mvpID, 1, GL_FALSE, &mvpMatrix[0][0]);

    const GLint colorID = glGetUniformLocation(lineShaderID, "color");
    constexpr glm::vec3 color = {1, 1, 1};
    glUniform3f(colorID, color.r, color.g, color.b);

    outlinesVao->enable();
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

    glUseProgram(cubeShaderID);
}

void OpenGLRenderer::finishRendering() const {
    glFlush();
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void OpenGLRenderer::debugCallback(const GLenum source, const GLenum type, const GLuint id, const GLenum severity,
                                   const GLsizei length, const GLchar *message, const void *userParam) {
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

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        std::cout << ss.str();
    } else {
        throw std::runtime_error(ss.str());
    }
}

void OpenGLRenderer::windowRefreshCallback(GLFWwindow *window) {
    // render();
    glfwSwapBuffers(window);
    glFinish(); // important, this waits until rendering result is actually visible, thus making resizing less ugly
}

void OpenGLRenderer::framebufferSizeCallback(GLFWwindow *window, const int width, const int height) {
    glViewport(0, 0, width, height);
}
