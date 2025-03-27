#pragma once

#include <string>
#include <map>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace Render {

struct Character {
    GLuint TextureID;       // ID of glyph texture
    glm::ivec2 Size;        // Size of glyph
    glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
    GLuint Advance;         // Horizontal offset to advance to next glyph
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
    void updateProjection(float width, float height);
    
    // Test rendering function
    void renderTestText();
    
    bool isInitialized() const {
        return shaderProgram && VAO && VBO && !Characters.empty();
    }

    float getTextWidth(const std::string& text, float scale) const;

    float getCharacterWidth(char c, float scale) const {
        auto it = Characters.find(c);
        if (it != Characters.end()) {
            return (it->second.Advance >> 6) * scale;
        }
        return 0.0f;
    }
    
private:
    // Fallback rendering method when regular text rendering fails
    void renderFallbackText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
    
    // Draw a simple character with lines
    void drawSimpleCharacter(char c, float x, float y, float width, float height);

    // OpenGL resources
    GLuint VAO;               // Vertex Array Object
    GLuint VBO;               // Vertex Buffer Object
    GLuint shaderProgram;     // Shader program for text rendering

    // Shader uniform locations
    GLint textUniformLoc;     // Location of text sampler uniform
    GLint textColorUniformLoc; // Location of text color uniform
    GLint projectionUniformLoc; // Location of projection matrix uniform

    // FreeType resources
    FT_Library ft;            // FreeType library instance
    FT_Face face;             // FreeType face object

    // Projection matrix
    glm::mat4 projection;

    // Character map
    std::map<GLchar, Character> Characters;
};

} // namespace Render 