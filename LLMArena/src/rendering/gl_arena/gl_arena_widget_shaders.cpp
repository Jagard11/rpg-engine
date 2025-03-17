// src/rendering/gl_arena/gl_arena_widget_shaders.cpp
#include "../../include/rendering/gl_arena_widget.h"
#include <QDebug>

// Vertex struct for passing to OpenGL
struct Vertex {
    QVector3D position;
    QVector3D normal;
    QVector2D texCoord;
};

// Shader source for basic rendering (walls, floor)
const char* basicVertexShaderSource =
    "#version 330 core\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec3 normal;\n"
    "layout(location = 2) in vec2 texCoord;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec3 fragNormal;\n"
    "out vec2 fragTexCoord;\n"
    "out vec3 fragPos;\n"
    "void main() {\n"
    "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "    fragNormal = mat3(transpose(inverse(model))) * normal;\n"
    "    fragTexCoord = texCoord;\n"
    "    fragPos = vec3(model * vec4(position, 1.0));\n"
    "}\n";

const char* basicFragmentShaderSource =
    "#version 330 core\n"
    "in vec3 fragNormal;\n"
    "in vec2 fragTexCoord;\n"
    "in vec3 fragPos;\n"
    "uniform vec3 objectColor;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    // Ambient lighting\n"
    "    float ambientStrength = 0.3;\n"
    "    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);\n"
    "    \n"
    "    // Diffuse lighting\n"
    "    vec3 norm = normalize(fragNormal);\n"
    "    vec3 lightDir = normalize(lightPos - fragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);\n"
    "    \n"
    "    // Specular lighting\n"
    "    float specularStrength = 0.5;\n"
    "    vec3 viewDir = normalize(viewPos - fragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);\n"
    "    \n"
    "    vec3 result = (ambient + diffuse + specular) * objectColor;\n"
    "    fragColor = vec4(result, 1.0);\n"
    "}\n";

// Simplified billboard shader source for more robust operation
const char* billboardVertexShaderSource =
    "#version 330 core\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec2 texCoord;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform vec3 cameraRight;\n"
    "uniform vec3 cameraUp;\n"
    "uniform vec3 billboardPos;\n"
    "uniform vec2 billboardSize;\n"
    "out vec2 fragTexCoord;\n"
    "void main() {\n"
    "    vec3 vertPos = billboardPos;\n"
    "    vertPos += cameraRight * position.x * billboardSize.x;\n"
    "    vertPos += cameraUp * position.y * billboardSize.y;\n"
    "    gl_Position = projection * view * vec4(vertPos, 1.0);\n"
    "    fragTexCoord = texCoord;\n"
    "}\n";

const char* billboardFragmentShaderSource =
    "#version 330 core\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D textureSampler;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    vec4 texColor = texture(textureSampler, fragTexCoord);\n"
    "    // BUGFIX: Discard completely transparent fragments to avoid z-fighting\n"
    "    if (texColor.a < 0.01) discard;\n"
    "    fragColor = texColor;\n"
    "}\n";

// Shader source for grid
const char* gridVertexShaderSource =
    "#version 330 core\n"
    "layout(location = 0) in vec3 position;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "}\n";

const char* gridFragmentShaderSource =
    "#version 330 core\n"
    "uniform vec3 lineColor;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = vec4(lineColor, 0.5);\n"
    "}\n";

bool GLArenaWidget::initShaders()
{
    // BUGFIX: Cleanup any existing shader programs first to prevent leaks
    if (m_basicProgram) {
        delete m_basicProgram;
        m_basicProgram = nullptr;
    }
    if (m_billboardProgram) {
        delete m_billboardProgram;
        m_billboardProgram = nullptr;
    }
    if (m_gridProgram) {
        delete m_gridProgram;
        m_gridProgram = nullptr;
    }
    
    // Create and compile basic shader program
    m_basicProgram = new QOpenGLShaderProgram(this);
    m_basicProgram->setObjectName("basicProgram");
    
    if (!m_basicProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, basicVertexShaderSource)) {
        qWarning() << "Failed to compile basic vertex shader:" << m_basicProgram->log();
        return false;
    }
    
    if (!m_basicProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, basicFragmentShaderSource)) {
        qWarning() << "Failed to compile basic fragment shader:" << m_basicProgram->log();
        return false;
    }
    
    if (!m_basicProgram->link()) {
        qWarning() << "Failed to link basic shader program:" << m_basicProgram->log();
        return false;
    }
    qDebug() << "Basic shader program linked successfully";
    
    // Create and compile billboard shader program
    m_billboardProgram = new QOpenGLShaderProgram(this);
    m_billboardProgram->setObjectName("billboardProgram");
    
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, billboardVertexShaderSource)) {
        qWarning() << "Failed to compile billboard vertex shader:" << m_billboardProgram->log();
        return false;
    }
    qDebug() << "Billboard vertex shader compiled successfully";
    
    if (!m_billboardProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, billboardFragmentShaderSource)) {
        qWarning() << "Failed to compile billboard fragment shader:" << m_billboardProgram->log();
        return false;
    }
    qDebug() << "Billboard fragment shader compiled successfully";
    
    if (!m_billboardProgram->link()) {
        qWarning() << "Failed to link billboard shader program:" << m_billboardProgram->log();
        return false;
    }
    qDebug() << "Billboard shader program linked successfully";
    
    // List all uniform locations in billboard program for debugging
    qDebug() << "Billboard Shader Uniforms:";
    QList<QByteArray> uniformNames = {
        "view", "projection", "cameraRight", "cameraUp", 
        "billboardPos", "billboardSize", "textureSampler"
    };
    
    for (const QByteArray &name : uniformNames) {
        int loc = m_billboardProgram->uniformLocation(name);
        qDebug() << "  Uniform" << name << "location:" << loc;
    }
    
    // Create and compile grid shader program
    m_gridProgram = new QOpenGLShaderProgram(this);
    m_gridProgram->setObjectName("gridProgram");
    
    if (!m_gridProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShaderSource)) {
        qWarning() << "Failed to compile grid vertex shader:" << m_gridProgram->log();
        return false;
    }
    
    if (!m_gridProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShaderSource)) {
        qWarning() << "Failed to compile grid fragment shader:" << m_gridProgram->log();
        return false;
    }
    
    if (!m_gridProgram->link()) {
        qWarning() << "Failed to link grid shader program:" << m_gridProgram->log();
        return false;
    }
    qDebug() << "Grid shader program linked successfully";
    
    return true;
}