// src/arena/ui/gl_widgets/character_sprite.h
#ifndef CHARACTER_SPRITE_H
#define CHARACTER_SPRITE_H

#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QVector3D>

// Simple billboard sprite class for characters
class CharacterSprite : protected QOpenGLFunctions {
public:
    CharacterSprite();
    ~CharacterSprite();
    
    void init(QOpenGLContext* context, const QString& texturePath, 
              double width, double height, double depth);
    void render(QOpenGLShaderProgram* program, QMatrix4x4& viewMatrix, QMatrix4x4& projectionMatrix);
    void updatePosition(float x, float y, float z);
    
    // Getters for properties
    float width() const { return m_width; }
    float height() const { return m_height; }
    float depth() const { return m_depth; }
    QVector3D getPosition() const { return m_position; }
    
    // Safe getters with null checks
    bool hasValidTexture() const { return m_texture && m_texture->isCreated(); }
    bool hasValidVAO() const { return m_vao.isCreated(); }
    QOpenGLTexture* getTexture() const { return m_texture; }
    QOpenGLVertexArrayObject* getVAO() { return &m_vao; }
    
private:
    QOpenGLTexture* m_texture;
    QVector3D m_position;
    float m_width, m_height, m_depth;
    
    // Rendering data
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLVertexArrayObject m_vao;
};

#endif // CHARACTER_SPRITE_H