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

#include "gui.h"
#include "gl/gl-shader.h"

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
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

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
    glDepthFunc(GL_LEQUAL);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(&debugCallback), nullptr);

    glfwSetWindowRefreshCallback(window, windowRefreshCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // load & compile shaders
    cubeShader = std::make_unique<GLShader>("cube-shader.vert", "cube-shader.frag");
    skyboxShader = std::make_unique<GLShader>("skybox-shader.vert", "skybox-shader.frag");
    lineShader = std::make_unique<GLShader>("line-shader.vert", "line-shader.frag");
    depthShader = std::make_unique<GLShader>("depth-shader.vert", "depth-shader.frag");
    debugDepthShader = std::make_unique<GLShader>("debug-depth-quad.vert", "debug-depth-quad.frag");
    cubeShader->enable();

    loadTextures();

    // init VAOs
    chunksVao = std::make_unique<ChunksVertexArray>();

    skybox.vao = std::make_unique<BasicVertexArray>();
    skybox.vao->writeToBuffers(skyboxVerticesList);

    outlinesVao = std::make_unique<BasicVertexArray>();

    // init peripheral structures
    camera = std::make_unique<Camera>(window);

    depthMap = std::make_unique<GLFrameBuffer>();
    depthMap->attachDepth(depthMapSize);
}

OpenGLRenderer::~OpenGLRenderer() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void OpenGLRenderer::tick(const float deltaTime) const {
    camera->tick(deltaTime);
}

void OpenGLRenderer::loadTextures() const {
    using FacePathMapping = FaceMapping<std::filesystem::path>;

    const std::unordered_map<EBlockType, FacePathMapping> blockTexturePathMappings{
        {
            BlockType_Grass, FacePathMapping(
                std::make_pair(ALL_SIDE_FACES, "grass-side.png"),
                std::make_pair(Top, "grass-top.png"),
                std::make_pair(Bottom, "dirt.png")
            )
        },
        {
            BlockType_Dirt, FacePathMapping(
                std::make_pair(ALL_FACES, "dirt.png")
            )
        },
        {
            BlockType_Stone, FacePathMapping(
                std::make_pair(ALL_FACES, "stone.png")
            )
        }
    };

    const FacePathMapping skyboxTexturePaths{
        std::make_pair(ALL_SIDE_FACES, "sky-side.png"),
        std::make_pair(Top, "sky-top.png"),
        std::make_pair(Bottom, "sky-bottom.png")
    };

    textureManager->loadBlockTextures(blockTexturePathMappings);
    textureManager->loadSkyboxTextures(skyboxTexturePaths);
}

void OpenGLRenderer::startRendering() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    viewMatrix = camera->getViewMatrix();
    projectionMatrix = camera->getProjectionMatrix();
    vpMatrix = projectionMatrix * viewMatrix;

    cubeShader->enable();
    cubeShader->setUniform("LightDirection_worldspace", skybox.lightDirection);
    cubeShader->setUniform("doDrawShadows", shadowConfig.doDrawShadows);

    for (auto &vertices: tempLineVertexGroups | std::views::values) {
        vertices.clear();
    }
}

void OpenGLRenderer::setIsCursorLocked(const bool b) const {
    camera->setIsCursorLocked(b);
    glfwSetInputMode(window, GLFW_CURSOR, b ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void OpenGLRenderer::renderGuiSection() {
    constexpr auto sectionFlags = ImGuiTreeNodeFlags_DefaultOpen;

    if (ImGui::CollapsingHeader("Renderer ", sectionFlags)) {
        ImGui::Text("Sun direction: ");
        ImGui::SameLine();
        ImGui::PushItemWidth(50.0f);
        ImGui::DragFloat("X", &skybox.lightDirection.x, 0.01f, -1.0f, 1.0f, "%.2f");
        ImGui::SameLine();
        ImGui::DragFloat("Y", &skybox.lightDirection.y, 0.01f, -1.0f, 1.0f, "%.2f");
        ImGui::SameLine();
        ImGui::DragFloat("Z", &skybox.lightDirection.z, 0.01f, -1.0f, 1.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::Text("Shadows: ");
        ImGui::Checkbox("draw?", &shadowConfig.doDrawShadows);
        ImGui::DragFloat("frustum radius", &shadowConfig.frustumRadius, 1.f, 0.f, 1000.f, "%.0f");
        ImGui::DragFloat("near plane", &shadowConfig.nearPlane, 0.1f, 0.f, 1000.f, "%.1f");
        ImGui::DragFloat("far plane", &shadowConfig.farPlane, 1.f, 0.f, 1000.f, "%.0f");
        ImGui::DragFloat("light distance", &shadowConfig.lightDistance, 1.f, 0.f, 1000.f, "%.0f");
    }

    camera->renderGuiSection();
}

void OpenGLRenderer::renderSkybox() const {
    glDepthMask(GL_FALSE);
    skyboxShader->enable();

    skyboxShader->setUniform("V", camera->getStaticViewMatrix());
    skyboxShader->setUniform("P", projectionMatrix);
    skyboxShader->setUniform("LightDirection_worldspace", skybox.lightDirection);

    textureManager->bindSkyboxTextures(*skyboxShader);

    skybox.vao->enable();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(skyboxVerticesList.size()));
    glDepthMask(GL_TRUE);
}

void OpenGLRenderer::makeChunksShadowMap(const std::vector<Chunk::ChunkID>& targets) {
    // todo - make these dependent on the render distance
    const auto [doDrawShadows, frustumRadius, nearPlane, farPlane, lightDistance] = shadowConfig;

    const glm::mat4 lightProjection = glm::ortho(
        -frustumRadius, frustumRadius,
        -frustumRadius, frustumRadius,
        nearPlane, farPlane
    );

    const glm::vec3 lightOffset =
        VecUtils::floor(camera->getPos() * (1.0f / Chunk::CHUNK_SIZE)) * static_cast<float>(Chunk::CHUNK_SIZE);
    const glm::mat4 lightView = glm::lookAt(
        lightOffset + glm::normalize(skybox.lightDirection) * lightDistance,
        lightOffset,
        glm::vec3(0.f, 1.f, 0.f)
    );

    glViewport(0, 0, depthMapSize.x, depthMapSize.y);

    depthMap->enable();

    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader->enable();
    lightVpMatrix = lightProjection * lightView;
    depthShader->setUniform("MVP", lightVpMatrix);

    chunksVao->enable();
    chunksVao->render(targets); // todo idea: maybe redraw only if something changed?

    depthMap->disable();

    glm::ivec2 windowSize;
    glfwGetWindowSize(window, &windowSize.x, &windowSize.y);
    glViewport(0, 0, windowSize.x, windowSize.y);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::renderChunks(const std::vector<Chunk::ChunkID>& targets) const {
    cubeShader->enable();
    textureManager->bindBlockTextures(*cubeShader);

    // todo - move to tex manager
    glActiveTexture(GL_TEXTURE0 + textureManager->getNextFreeUnit());
    glBindTexture(GL_TEXTURE_2D, depthMap->getDepth());
    cubeShader->setUniform("shadowMap", textureManager->getNextFreeUnit());

    cubeShader->setUniform("MVP", vpMatrix); // M is the identity matrix
    cubeShader->setUniform("lightMVP", lightVpMatrix); // M is the identity matrix

    chunksVao->enable();
    chunksVao->render(targets);
}

void OpenGLRenderer::renderOutlines() {
    lineShader->enable();
    lineShader->setUniform("MVP", vpMatrix); // model matrix is the identity matrix so no need to multiply it in

    for (const auto &[gid, vertices]: tempLineVertexGroups) {
        outlinesVao->writeToBuffers(vertices);

        lineShader->setUniform("color", vertexGroupColors.at(gid));

        outlinesVao->enable();
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    }
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

void OpenGLRenderer::addChunkOutline(const glm::ivec3 &chunkPos, const LineType gid) {
    addCubeOutline(chunkPos, Chunk::CHUNK_SIZE * Block::RENDER_SIZE, gid);
}

auto OpenGLRenderer::addTargetedBlockOutline(const glm::ivec3 &blockPos) -> void {
    addCubeOutline(blockPos, Block::RENDER_SIZE, SELECTED_BLOCK_OUTLINE);
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

    glm::ivec2 windowSize;
    glfwGetWindowSize(window, &windowSize.x, &windowSize.y);

    vertices.push_back(top * static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y));
    vertices.push_back(bottom * static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y));

    glClear(GL_DEPTH_BUFFER_BIT);

    lineShader->enable();

    outlinesVao->writeToBuffers(vertices);

    lineShader->setUniform("MVP", glm::identity<glm::mat4>());
    lineShader->setUniform("color", glm::vec3(1));

    outlinesVao->enable();
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

    cubeShader->enable();
}

void OpenGLRenderer::finishRendering() const {
    //glFlush();
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
    if (width && height) {
        glViewport(0, 0, width, height);
    }
}
