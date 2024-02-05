#ifndef VOXEL_TEXTURE_MANAGER_H
#define VOXEL_TEXTURE_MANAGER_H

#include "GL/glew.h"
#include "src/voxel/block.h"
#include <map>
#include <filesystem>

/**
 * Class managing all the textures used by the renderer.
 * This currently includes only textures for blocks as well as for text.
 */
class TextureManager {
    using TextureMap = std::map<std::pair<EBlockType, EBlockFace>, GLuint>;
    TextureMap blockTextures;

    GLuint skyboxCubemap{};

public:
    using BlockTexPathMapping = const std::map<EBlockType, FaceMapping<std::filesystem::path>>;

    /**
     * Loads all given block textures at given paths and applies them to all faces of given block types.
     *
     * @param blockTexPathMappings Map containing paths to texture for each face of each block.
     */
    void loadBlockTextures(const BlockTexPathMapping& blockTexPathMappings);

    /**
     * Loads a texture at a given path and applies it to the skybox.
     *
     * @param skyboxTexturePaths Paths to files containing the textures.
     */
    void loadSkyboxTextures(const FaceMapping<std::filesystem::path>& skyboxTexturePaths);

    /**
     * Binds all managed block textures so that they can be used by the provided shader.
     *
     * @param blockShaderID ID of the shader program which will use all the textures.
     */
    void bindBlockTextures(GLuint blockShaderID) const;

    /**
     * Binds all managed skybox textures so that they can be used by the provided shader.
     *
     * @param skyboxShaderID ID of the shader program which will use all the textures.
     */
    void bindSkyboxTextures(GLuint skyboxShaderID) const;

    [[nodiscard]]
    GLuint getBlockTextureID(EBlockType blockType, EBlockFace face) const;

    [[nodiscard]]
    static int getBlockSamplerID(EBlockType blockType, EBlockFace face);

private:
    /**
     * Loads a texture stored inside a given file.
     *
     * @param path Path to the file.
     * @return OpenGL handle to the newly loaded texture.
     */
    static GLuint loadTexture(const std::filesystem::path& path);

    /**
     * Loads a cubemap texture stored inside a given file.
     *
     * @param skyboxTexturePaths Paths to texture files.
     * @return OpenGL handle to the newly loaded cubemap texture.
     */
    static GLuint loadCubemapTexture(const FaceMapping<std::filesystem::path>& skyboxTexturePaths);

    /**
     * Loads a texture stored inside a given .DDS file.
     *
     * @param path Path to the file.
     * @return OpenGL handle to the newly loaded texture.
     */
    static GLuint loadTextureDDS(const std::filesystem::path &path);

    /**
     * Loads a cubemap texture stored inside a given .DDS file.
     *
     * @param skyboxTexturePaths Paths to texture files.
     * @return OpenGL handle to the newly loaded cubemap texture.
     */
    static GLuint loadCubemapDDS(const FaceMapping<std::filesystem::path>& skyboxTexturePaths);
};

#endif //VOXEL_TEXTURE_MANAGER_H
