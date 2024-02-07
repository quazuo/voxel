#include "texture-manager.h"

#include <fstream>
#include <cstring>
#include <vector>
#include <array>
#include <filesystem>

#include "deps/stb/stb_image.h"

static constexpr std::array blockFaces = {
    EBlockFace::Front,
    EBlockFace::Back,
    EBlockFace::Right,
    EBlockFace::Left,
    EBlockFace::Top,
    EBlockFace::Bottom,
};

void TextureManager::loadBlockTextures(const BlockTexPathMapping &blockTexPathMappings) {
    for (const auto &[block, faceTexPathMapping]: blockTexPathMappings) {
        // if all faces are textured the same, just load one texture and share it
        if (faceTexPathMapping.contains(ALL_FACES)) {
            const auto texID = loadTexture(faceTexPathMapping.get(Front)); // just take whatever face

            for (auto face: blockFaces) {
                blockTextures[{block, face}] = texID;
            }

            continue;
        }

        // work similarly with side faces
        if (faceTexPathMapping.contains(ALL_SIDE_FACES)) {
            const auto texID = loadTexture(faceTexPathMapping.get(Front)); // just take whatever side face

            for (auto face: blockFaces) {
                if (face & ALL_SIDE_FACES) {
                    blockTextures[{block, face}] = texID;
                }
            }
        } else {
            for (auto face: blockFaces) {
                if (face & ALL_SIDE_FACES) {
                    blockTextures[{block, face}] = loadTexture(faceTexPathMapping.get(face));
                }
            }
        }

        blockTextures[{block, Top}] = loadTexture(faceTexPathMapping.get(Top));
        blockTextures[{block, Bottom}] = loadTexture(faceTexPathMapping.get(Bottom));
    }
}

void TextureManager::loadSkyboxTextures(const FaceMapping<std::filesystem::path> &skyboxTexturePaths) {
    skyboxCubemap = loadCubemapTexture(skyboxTexturePaths);
}

static int getFaceIdOffset(const EBlockFace face) {
    switch (face) {
        case Front:
            return 0;
        case Back:
            return 1;
        case Right:
            return 2;
        case Left:
            return 3;
        case Top:
            return 4;
        case Bottom:
            return 5;
        default:
            throw std::runtime_error("invalid switch branch in getFaceIdOffset");
    }
}

int TextureManager::getBlockSamplerID(const EBlockType blockType, const EBlockFace face) const {
    const GLuint texID = blockTextures.at({blockType, face});
    return textureUnits.at(texID);
}

void TextureManager::bindBlockTextures(const GLuint blockShaderID) const {
    std::vector<GLint> handles(nextFreeUnit);

    for (auto &[key, id]: blockTextures) {
        auto &[type, face] = key;
        const int samplerID = getBlockSamplerID(type, face);

        glActiveTexture(GL_TEXTURE0 + samplerID);
        glBindTexture(GL_TEXTURE_2D, id);
        handles[samplerID] = samplerID;
    }

    glUniform1iv(
        glGetUniformLocation(blockShaderID, "texSampler"),
        static_cast<GLint>(blockTextures.size()),
        handles.data()
    );
}

void TextureManager::bindSkyboxTextures(const GLuint skyboxShaderID) const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    glUniform1i(glGetUniformLocation(skyboxShaderID, "skyboxTexSampler"), 0);
}

GLuint TextureManager::getBlockTextureID(const EBlockType blockType, const EBlockFace face) const {
    if (!blockTextures.contains({blockType, face})) {
        throw std::runtime_error("tried to call getTextureIDs() on uninitialized texture data");
    }

    return blockTextures.at({blockType, face});
}

// --------------------- texture loading ---------------------

struct TextureData {
    int width, height, nrChannels;
    unsigned char *data;
};

static TextureData readTexture(const std::filesystem::path &path) {
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.string().c_str(), &width, &height, &nrChannels, 0);

    if (!data) {
        throw std::runtime_error("stbi_load failed in readTexture");
    }

    return {width, height, nrChannels, data};
}

GLuint TextureManager::loadTexture(const std::filesystem::path &path) {
    if (loadedTextures.contains(path.string())) {
        return loadedTextures.at(path.string());
    }

    const auto &[width, height, nrChannels, data] = readTexture(path);

    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(data);

    loadedTextures.emplace(path.string(), texID);
    textureUnits.emplace(texID, nextFreeUnit++);

    return texID;
}

static int getCubemapSide(const EBlockFace face) {
    switch (face) {
        case Front:
            return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        case Back:
            return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        case Right:
            return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        case Left:
            return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        case Top:
            return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        case Bottom:
            return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        default:
            throw std::runtime_error("invalid switch branch in getCubemapSide");
    }
}

GLuint TextureManager::loadCubemapTexture(const FaceMapping<std::filesystem::path> &skyboxTexturePaths) {
    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    for (const auto face: blockFaces) {
        const auto path = skyboxTexturePaths.get(face);
        const auto &[width, height, nrChannels, data] = readTexture(path);

        glTexImage2D(getCubemapSide(face), 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texID;
}

// --------------------- DDS loading ---------------------

struct DDSFileContent {
    uint32_t height, width, linearSize, mipMapCount, format;
    std::vector<char> buffer;
};

static DDSFileContent readDDS(const std::filesystem::path &path) {
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
    }

    constexpr size_t headerSize = 124;
    char header[headerSize];
    fileStream.read(header, headerSize);

    const uint32_t height = *reinterpret_cast<uint32_t *>(&(header[8]));
    const uint32_t width = *reinterpret_cast<uint32_t *>(&(header[12]));
    const uint32_t linearSize = *reinterpret_cast<uint32_t *>(&(header[16]));
    const uint32_t mipMapCount = *reinterpret_cast<uint32_t *>(&(header[24]));
    const uint32_t fourCC = *reinterpret_cast<uint32_t *>(&(header[80]));

    const unsigned int bufsize = mipMapCount > 1
                                     ? linearSize * 2
                                     : linearSize;
    std::vector<char> buffer(bufsize);
    fileStream.read(&buffer[0], bufsize);

    fileStream.close();

    static constexpr uint32_t FOURCC_DXT1 = 0x31545844;
    static constexpr uint32_t FOURCC_DXT3 = 0x33545844;
    static constexpr uint32_t FOURCC_DXT5 = 0x35545844;

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
            throw std::runtime_error("unrecognized DXT format in DDS file " + path.string());
    }

    return {height, width, linearSize, mipMapCount, format, buffer};
}

GLuint TextureManager::loadTextureDDS(const std::filesystem::path &path) {
    auto [height, width, linearSize, mipMapCount, format, buffer] = readDDS(path);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    // load the mipmaps
    for (unsigned int level = 0; level < mipMapCount && (width || height); level++) {
        const unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
        glCompressedTexImage2D(
            GL_TEXTURE_2D,
            static_cast<GLint>(level),
            format,
            static_cast<GLsizei>(width),
            static_cast<GLsizei>(height),
            0,
            static_cast<GLsizei>(size),
            buffer.data() + offset
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

    return textureID;
}

GLuint TextureManager::loadCubemapDDS(const FaceMapping<std::filesystem::path> &skyboxTexturePaths) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (const auto face: blockFaces) {
        const auto path = skyboxTexturePaths.get(face);
        const auto [height, width, linearSize, mipMapCount, format, buffer] = readDDS(path);
        const unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;

        uint32_t w = width, h = height;
        unsigned int offset = 0;

        for (unsigned int level = 0; level < mipMapCount && (w || h); level++) {
            const unsigned int size = ((w + 3) / 4) * ((h + 3) / 4) * blockSize;
            glCompressedTexImage2D(
                getCubemapSide(face),
                static_cast<GLint>(level),
                format,
                static_cast<GLsizei>(w),
                static_cast<GLsizei>(h),
                0,
                static_cast<GLsizei>(size),
                buffer.data() + offset
            );

            offset += size;
            w /= 2;
            h /= 2;

            // deal with non-power-of-two textures
            if (w < 1) w = 1;
            if (h < 1) h = 1;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
