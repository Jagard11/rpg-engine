#include "renderer/TextureManager.hpp"
#include <stb_image.h>
#include <iostream>

TextureManager::TextureManager() {
}

TextureManager::~TextureManager() {
    for (GLuint textureID : m_textureIDs) {
        glDeleteTextures(1, &textureID);
    }
}

bool TextureManager::initialize() {
    // Load textures
    const std::string textureFiles[] = {
        "resources/dirt.png",
        "resources/grass.png",
        "resources/cobblestone.png"
    };

    for (const auto& filename : textureFiles) {
        GLuint textureID;
        if (!loadTexture(filename, textureID)) {
            std::cerr << "Failed to load texture: " << filename << std::endl;
            return false;
        }
        m_textureIDs.push_back(textureID);
    }

    return true;
}

bool TextureManager::loadTexture(const std::string& filename, GLuint& textureID) {
    // Generate texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return false;
    }

    // Upload texture data
    GLenum format = channels == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Free image data
    stbi_image_free(data);

    return true;
}

GLuint TextureManager::getTextureID(int index) const {
    if (index >= 0 && index < m_textureIDs.size()) {
        return m_textureIDs[index];
    }
    return 0;
}

void TextureManager::bindTexture(int index) {
    if (index >= 0 && index < m_textureIDs.size()) {
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[index]);
    }
} 