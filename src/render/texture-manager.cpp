#include "texture-manager.h"

#include <fstream>
#include <cstring>
#include <vector>
#include <array>
#include <filesystem>

void TextureManager::loadBlockTexture(const EBlockType blockType, const std::uint8_t faces,
                                      const std::filesystem::path &path) {
    static constexpr std::array blockFaces = {
        EBlockFace::Front,
        EBlockFace::Back,
        EBlockFace::Right,
        EBlockFace::Left,
        EBlockFace::Top,
        EBlockFace::Bottom,
    };

    for (auto face: blockFaces) {
        if (faces & face && blockTextures.contains({blockType, face}))
            return;
    }

    const GLuint texID = loadDDS(path);

    for (auto face: blockFaces) {
        if (faces & face) {
            blockTextures[{blockType, face}] = texID;
        }
    }
}

void TextureManager::loadFontTexture(const std::filesystem::path &path) {
    fontTexture = loadDDS(path);
}

int TextureManager::getBlockSamplerID(const EBlockType blockType, const EBlockFace face) {
    static const std::map<EBlockFace, int> faceIdOffsets = {
        {Front, 0},
        {Back, 1},
        {Right, 2},
        {Left, 3},
        {Top, 4},
        {Bottom, 5}
    };

    return static_cast<int>(EBlockFace::N_FACES) * (blockType - 1) + faceIdOffsets.at(face);
}

void TextureManager::bindBlockTextures(const GLuint blockShaderID) const {
    std::vector<GLint> handles(blockTextures.size());

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
        &handles[0]
    );
}

void TextureManager::bindFontTexture(const GLuint textShaderID) const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glUniform1i(glGetUniformLocation(textShaderID, "texSampler"), 0);
}

GLuint TextureManager::getBlockTextureID(const EBlockType blockType, const EBlockFace face) const {
    if (!blockTextures.contains({blockType, face})) {
        throw std::runtime_error("tried to call getTextureIDs() on uninitialized texture data");
    }

    return blockTextures.at({blockType, face});
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

    uint32_t height = *reinterpret_cast<uint32_t *>(&(header[8]));
    uint32_t width = *reinterpret_cast<uint32_t *>(&(header[12]));
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

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    // load the mipmaps
    for (unsigned int level = 0; level < mipMapCount && (width || height); ++level) {
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
