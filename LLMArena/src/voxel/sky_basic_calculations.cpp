// src/voxel/sky_basic_calculations.cpp
#include "../../include/voxel/sky_system_helpers.h"
#include <QVector3D>
#include <cmath>

// Simple sun position calculation without astronomical formulas
QVector3D calculateSunPositionSimple(float skyboxRadius, const QDateTime& time) {
    // Extract hour and minute as decimal
    int hour = time.time().hour();
    int minute = time.time().minute();
    float timeOfDay = hour + minute / 60.0f;
    
    // Calculate angle (0 at midnight, π at noon)
    float angle = (timeOfDay / 24.0f) * 2.0f * M_PI;
    
    // Sun position on a circle
    float x = skyboxRadius * 0.8f * cos(angle);
    float y = skyboxRadius * 0.8f * sin(angle);
    float z = 0.0f;
    
    // Adjust for more realistic path
    if (y < 0) {
        y *= 0.3f; // Flatten when sun is below horizon
    }
    
    return QVector3D(x, y, z);
}

// Simple moon position calculation without astronomical formulas
QVector3D calculateMoonPositionSimple(float skyboxRadius, const QDateTime& time) {
    // Moon is roughly opposite to sun
    QVector3D sunPos = calculateSunPositionSimple(skyboxRadius, time);
    
    // Offset to avoid exact opposition
    float angleOffset = 0.2f * M_PI;
    
    // Calculate moon position
    float x = -sunPos.x() * 0.9f * cos(angleOffset);
    float y = -sunPos.y() * 0.9f * sin(angleOffset);
    float z = skyboxRadius * 0.1f * sin(angleOffset); // Slight z-offset
    
    return QVector3D(x, y, z);
}