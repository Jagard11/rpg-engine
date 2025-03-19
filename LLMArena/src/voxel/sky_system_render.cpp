// src/voxel/sky_system_render.cpp
#include "../../include/voxel/sky_system.h"
#include <QDebug>

void SkySystem::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix) {
    // Check if necessary components exist
    if (!m_skyboxShader || !m_celestialShader) {
        return;
    }
    
    // Check for valid VAOs
    if (!m_skyboxVAO.isCreated() || !m_celestialVAO.isCreated()) {
        return;
    }
    
    try {
        // Enable depth testing but disable depth writing for skybox
        glDepthMask(GL_FALSE);
        
        // Render skybox
        if (m_skyboxShader && m_skyboxShader->bind()) {
            // Set sky color
            QVector3D skyColorVec(m_skyColor.redF(), m_skyColor.greenF(), m_skyColor.blueF());
            if (m_skyboxShader->uniformLocation("skyColor") != -1)
                m_skyboxShader->setUniformValue("skyColor", skyColorVec);
            
            // Draw skybox as a screen-space quad
            m_skyboxVAO.bind();
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            m_skyboxVAO.release();
            
            m_skyboxShader->release();
        }
        
        // Re-enable depth writing
        glDepthMask(GL_TRUE);
        
        // Render sun and moon as billboards
        if (m_celestialShader && m_celestialShader->bind()) {
            // Sun
            if (m_sunPosition.y() > -m_skyboxRadius * 0.2f && m_sunTexture && m_sunTexture->isCreated()) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                // Get camera position from view matrix
                QMatrix4x4 invView = viewMatrix.inverted();
                QVector3D cameraPos = invView * QVector3D(0, 0, 0);
                
                // Calculate billboard orientation (fixed to always face camera)
                QMatrix4x4 modelMatrix;
                modelMatrix.setToIdentity();
                modelMatrix.translate(m_sunPosition);
                
                // Calculate the direction from the billboard to the camera
                QVector3D dir = (cameraPos - m_sunPosition).normalized();
                
                // Create rotation matrix that aligns billboard to face camera
                // Use camera up vector for more stable orientation
                QVector3D up(0, 1, 0);
                QVector3D right = QVector3D::crossProduct(dir, up).normalized();
                up = QVector3D::crossProduct(right, dir).normalized();
                
                // Apply the camera-facing rotation
                modelMatrix.scale(m_sunRadius * 2.0f);
                
                // Use billboarding matrix to always face camera
                QMatrix4x4 billboardMatrix;
                billboardMatrix.setToIdentity();
                billboardMatrix.setColumn(0, QVector4D(right, 0.0f));
                billboardMatrix.setColumn(1, QVector4D(up, 0.0f));
                billboardMatrix.setColumn(2, QVector4D(-dir, 0.0f)); // Negative because we want it to face the camera
                billboardMatrix.setColumn(3, QVector4D(0, 0, 0, 1.0f));
                
                modelMatrix = modelMatrix * billboardMatrix;
                
                // Set uniforms
                m_celestialShader->setUniformValue("model", modelMatrix);
                m_celestialShader->setUniformValue("view", viewMatrix);
                m_celestialShader->setUniformValue("projection", projectionMatrix);
                m_celestialShader->setUniformValue("textureSampler", 0);
                
                // Bind texture
                glActiveTexture(GL_TEXTURE0);
                m_sunTexture->bind();
                
                // Draw sun
                m_celestialVAO.bind();
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                m_celestialVAO.release();
                
                m_sunTexture->release();
            }
            
            // Moon
            if (m_moonPosition.y() > -m_skyboxRadius * 0.2f && m_moonTexture && m_moonTexture->isCreated()) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                // Get camera position from view matrix
                QMatrix4x4 invView = viewMatrix.inverted();
                QVector3D cameraPos = invView * QVector3D(0, 0, 0);
                
                // Calculate billboard orientation (fixed to always face camera)
                QMatrix4x4 modelMatrix;
                modelMatrix.setToIdentity();
                modelMatrix.translate(m_moonPosition);
                
                // Calculate the direction from the billboard to the camera
                QVector3D dir = (cameraPos - m_moonPosition).normalized();
                
                // Create rotation matrix that aligns billboard to face camera
                // Use camera up vector for more stable orientation
                QVector3D up(0, 1, 0);
                QVector3D right = QVector3D::crossProduct(dir, up).normalized();
                up = QVector3D::crossProduct(right, dir).normalized();
                
                // Apply the camera-facing rotation
                modelMatrix.scale(m_moonRadius * 2.0f);
                
                // Use billboarding matrix to always face camera
                QMatrix4x4 billboardMatrix;
                billboardMatrix.setToIdentity();
                billboardMatrix.setColumn(0, QVector4D(right, 0.0f));
                billboardMatrix.setColumn(1, QVector4D(up, 0.0f));
                billboardMatrix.setColumn(2, QVector4D(-dir, 0.0f)); // Negative because we want it to face the camera
                billboardMatrix.setColumn(3, QVector4D(0, 0, 0, 1.0f));
                
                modelMatrix = modelMatrix * billboardMatrix;
                
                // Set uniforms
                m_celestialShader->setUniformValue("model", modelMatrix);
                m_celestialShader->setUniformValue("view", viewMatrix);
                m_celestialShader->setUniformValue("projection", projectionMatrix);
                m_celestialShader->setUniformValue("textureSampler", 0);
                
                // Bind texture
                glActiveTexture(GL_TEXTURE0);
                m_moonTexture->bind();
                
                // Draw moon
                m_celestialVAO.bind();
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
                m_celestialVAO.release();
                
                m_moonTexture->release();
            }
            
            m_celestialShader->release();
        }
    } catch (...) {
        // Silent exception handling
    }
}