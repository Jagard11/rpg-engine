#pragma once

#include <string>
#include <map>
#include <GL/glew.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Debug {

class TextRenderer {
private:
    struct Character {
        GLuint     TextureID;  // ID handle of the glyph texture
        glm::ivec2 Size;       // Size of glyph
        glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
        GLuint     Advance;    // Offset to advance to next glyph
    };

    // OpenGL resources
    GLuint VAO;               // Vertex Array Object
    GLuint VBO;               // Vertex Buffer Object
    GLuint shaderProgram;     // Shader program for text rendering

    // FreeType resources
    FT_Library ft;            // FreeType library instance
    FT_Face face;             // FreeType face object

    // Character map
    std::map<GLchar, Character> Characters;

public:
    TextRenderer();
    ~TextRenderer();
    void renderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, const glm::vec3& color);
    void renderCenteredText(const std::string& text, GLfloat y, GLfloat scale, const glm::vec3& color);
};

} // namespace Debug 