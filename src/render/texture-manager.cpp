#include "texture-manager.h"

#include <fstream>
#include <cstring>
#include <vector>
#include <iostream>

void TextureManager::loadBlockTexture(EBlockType tex, const std::filesystem::path &path) {
    if (blockTextures.contains(tex))
        return;

    blockTextures[tex] = loadDDS(path);
}

void TextureManager::loadFontTexture(const std::filesystem::path &path) {
    fontTexture = loadDDS(path);
}

void TextureManager::bindBlockTextures(const GLuint blockShaderID) const {
    std::vector<GLint> handles(blockTextures.size());

    for (auto &[blockType, id] : blockTextures) {
        glActiveTexture(GL_TEXTURE0 + blockType - 1);
        glBindTexture(GL_TEXTURE_2D, id);
        handles[blockType - 1] = blockType - 1;
    }

    glUniform1iv(glGetUniformLocation(blockShaderID, "texSampler"), (GLint) blockTextures.size(), &handles[0]);
}

void TextureManager::bindFontTexture(GLuint textShaderID) const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glUniform1i(glGetUniformLocation(textShaderID, "texSampler"), 0);
}

GLuint TextureManager::getBlockTextureID(EBlockType tex) const {
    if (!blockTextures.contains(tex)) {
        throw std::runtime_error("tried to call getTextureIDs() on uninitialized texture data");
    }

    return blockTextures.at(tex);
}

GLuint TextureManager::loadDDS(const std::filesystem::path &path) {
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

    uint32_t height = *(uint32_t *) &(header[8]);
    uint32_t width = *(uint32_t *) &(header[12]);
    uint32_t linearSize = *(uint32_t *) &(header[16]);
    uint32_t mipMapCount = *(uint32_t *) &(header[24]);
    uint32_t fourCC = *(uint32_t *) &(header[80]);

    unsigned int bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
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

    GLuint textureID;
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

    return textureID;
}
