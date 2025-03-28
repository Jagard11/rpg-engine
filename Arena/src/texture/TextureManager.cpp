#include "texture/TextureManager.hpp"
#include <stb_image.h>
#include <iostream>

TextureManager& TextureManager::getInstance() {
    static TextureManager instance;
    return instance;
}

GLuint TextureManager::loadTexture(const std::string& filename) {
    // Check if texture is already loaded
    auto it = m_textures.find(filename);
    if (it != m_textures.end()) {
        return it->second;
    }

    // Load image using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    // Generate and bind texture
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determine format based on number of channels
    GLenum format;
    switch (channels) {
        case 1:
            format = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            std::cerr << "Unsupported number of channels: " << channels << std::endl;
            stbi_image_free(data);
            return 0;
    }

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Free image data
    stbi_image_free(data);

    // Store texture ID
    m_textures[filename] = textureId;
    return textureId;
}

GLuint TextureManager::getTexture(const std::string& filename) const {
    auto it = m_textures.find(filename);
    return it != m_textures.end() ? it->second : 0;
}

void TextureManager::bindTexture(GLuint textureId, GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

void TextureManager::cleanup() {
    for (const auto& pair : m_textures) {
        glDeleteTextures(1, &pair.second);
    }
    m_textures.clear();
} 