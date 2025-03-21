// src/arena/skybox/astronomy/sun_position.cpp
#include "../../include/arena/skybox/sky_system.h"
#include "../../include/splash/ui/location_dialog.h"
#include <QSettings>
#include <QtMath>
#include <QDebug>
#include <QTimeZone>

// Forward declarations for simple calculation functions
// These are defined in sky_basic_calculations.cpp
QVector3D calculateSunPositionSimple(float skyboxRadius, const QDateTime& time);
QVector3D calculateMoonPositionSimple(float skyboxRadius, const QDateTime& time);

// Astronomical calculations for sun/moon positions
// These functions implement simplified versions of actual astronomical formulas

namespace {
    // Convert degrees to radians
    double toRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }
    
    // Convert radians to degrees
    double toDegrees(double radians) {
        return radians * 180.0 / M_PI;
    }
    
    // Calculate day of year (0-365)
    int dayOfYear(const QDateTime& date) {
        return date.date().dayOfYear() - 1;
    }
    
    // Calculate fractional hour (0-24)
    double fractionalHour(const QDateTime& date) {
        return date.time().hour() + date.time().minute() / 60.0 + date.time().second() / 3600.0;
    }
    
    // Calculate Julian Day from QDateTime
    double julianDay(const QDateTime& dateTime) {
        // Get date components
        QDate date = dateTime.date();
        int Y = date.year();
        int M = date.month();
        int D = date.day();
        
        // Get time components and convert to decimal day
        double h = dateTime.time().hour();
        double minutes = dateTime.time().minute();
        double s = dateTime.time().second();
        double ms = dateTime.time().msec();
        double dayFraction = (h + minutes/60.0 + s/3600.0 + ms/3600000.0) / 24.0;
        
        // Calculate Julian Day
        int a = (14 - M) / 12;
        int y = Y + 4800 - a;
        int adjustedMonth = M + 12*a - 3;
        
        double jd = D + (153*adjustedMonth + 2)/5 + 365*y + y/4 - y/100 + y/400 - 32045 + dayFraction;
        
        return jd;
    }
    
    // Calculate Julian Centuries since J2000.0
    double julianCentury(double jd) {
        return (jd - 2451545.0) / 36525.0;
    }
}

// Calculate sun position using declination and hour angle
QVector3D calculateSunPositionAstronomical(float skyboxRadius, const QDateTime& time) {
    // Get the current location data
    LocationData location = LocationDialog::loadLocation();
    if (location.name.isEmpty()) {
        // Use default formula if no location is set
        return calculateSunPositionSimple(skyboxRadius, time);
    }
    
    try {
        // Instead of complicated solar time calculations, use a simpler approach
        // Get local hour (0-24)
        double hour = fractionalHour(time);
        int doy = dayOfYear(time);
        
        // Calculate solar declination (approximate formula)
        double declination = 23.45 * qSin(toRadians(360.0/365.0 * (doy - 81)));
        
        // For hour angle, use the local hour but adjust for longitude
        // 15 degrees = 1 hour, and we need to account for the fact that
        // west longitudes are negative in solar time calculations
        // 12 is solar noon
        double hourAngle = 15.0 * (hour - 12.0);
        
        // Convert to radians
        double lat = toRadians(location.latitude);
        double decl = toRadians(declination);
        double ha = toRadians(hourAngle);
        
        // Calculate altitude and azimuth of the sun
        double sinAlt = qSin(lat) * qSin(decl) + qCos(lat) * qCos(decl) * qCos(ha);
        double altitude = qAsin(sinAlt);
        
        double cosAz = (qSin(decl) - qSin(lat) * sinAlt) / (qCos(lat) * qCos(altitude));
        // Make sure cosAz is in the valid range for acos
        cosAz = qBound(-1.0, cosAz, 1.0);
        double azimuth = qAcos(cosAz);
        
        // Correct azimuth based on hour angle
        if (qSin(ha) > 0) {
            azimuth = 2 * M_PI - azimuth;
        }
        
        // Convert altitude and azimuth to XYZ coordinates
        double distance = skyboxRadius * 0.8;
        double x = distance * qCos(altitude) * qSin(azimuth);
        double y = distance * qSin(altitude);
        double z = -distance * qCos(altitude) * qCos(azimuth); // Negate Z for OpenGL coordinates
        
        // Only log once per calculation, not twice
        static QDateTime lastLogTime;
        if (lastLogTime != time) {
            qDebug() << "Sun calculation:";
            qDebug() << "  Local Date/Time:" << time.toString();
            qDebug() << "  Location:" << location.name << location.latitude << location.longitude;
            qDebug() << "  DOY:" << doy << "Hour:" << hour;
            qDebug() << "  Declination:" << declination << "degrees";
            qDebug() << "  Hour Angle:" << hourAngle << "degrees";
            qDebug() << "  Altitude:" << toDegrees(altitude) << "degrees";
            qDebug() << "  Azimuth:" << toDegrees(azimuth) << "degrees";
            qDebug() << "  Position:" << x << y << z;
            
            lastLogTime = time;
        }
        
        return QVector3D(x, y, z);
    } 
    catch (const std::exception& e) {
        qCritical() << "Exception in sun position calculation:" << e.what();
        return QVector3D(0, skyboxRadius * 0.8, 0); // Default position (directly above)
    }
    catch (...) {
        qCritical() << "Unknown exception in sun position calculation";
        return QVector3D(0, skyboxRadius * 0.8, 0); // Default position (directly above)
    }
}

// Calculate moon position
QVector3D calculateMoonPositionAstronomical(float skyboxRadius, const QDateTime& time) {
    // Get the current location data
    LocationData location = LocationDialog::loadLocation();
    if (location.name.isEmpty()) {
        // Use default formula if no location is set
        return calculateMoonPositionSimple(skyboxRadius, time);
    }
    
    try {
        // Moon position is more complex than sun position
        // Here we'll use a very simplified model
        
        // Get local hour and day of year
        double hour = fractionalHour(time);
        int doy = dayOfYear(time);
        
        // Calculate Julian Day
        double jd = julianDay(time);
        double T = julianCentury(jd);
        
        // Mean orbital elements for the Moon
        double L0 = 218.316 + 481267.8813 * T;  // Mean longitude
        double M = 134.963 + 477198.8676 * T;   // Mean anomaly
        double F = 93.272 + 483202.0175 * T;    // Argument of latitude
        
        // Convert to radians and normalize
        L0 = toRadians(fmod(L0, 360.0));
        M = toRadians(fmod(M, 360.0));
        F = toRadians(fmod(F, 360.0));
        
        // Simplified formula for lunar declination
        double declination = 23.45 * qSin(F); // Very simplified
        
        // Lunar phase (0-1)
        double phaseAngle = fmod(L0 - M, 2 * M_PI);
        double phase = 0.5 * (1 - qCos(phaseAngle));
        
        // Hour angle calculation for moon (offset from sun by ~180°)
        // Shift by 12 hours to put moon roughly opposite the sun
        double shiftedHour = fmod(hour + 12, 24);
        double hourAngle = 15.0 * (shiftedHour - 12.0);
        
        // Convert to radians
        double lat = toRadians(location.latitude);
        double decl = toRadians(declination);
        double ha = toRadians(hourAngle);
        
        // Calculate altitude and azimuth of the moon
        double sinAlt = qSin(lat) * qSin(decl) + qCos(lat) * qCos(decl) * qCos(ha);
        double altitude = qAsin(sinAlt);
        
        double cosAz = (qSin(decl) - qSin(lat) * sinAlt) / (qCos(lat) * qCos(altitude));
        cosAz = qBound(-1.0, cosAz, 1.0); // Ensure within acos range
        double azimuth = qAcos(cosAz);
        
        if (qSin(ha) > 0) {
            azimuth = 2 * M_PI - azimuth;
        }
        
        // Convert altitude and azimuth to XYZ coordinates
        double distance = skyboxRadius * 0.7;
        double x = distance * qCos(altitude) * qSin(azimuth);
        double y = distance * qSin(altitude);
        double z = -distance * qCos(altitude) * qCos(azimuth); // Negate Z for OpenGL coordinates
        
        // Ensure the moon is not directly opposite the sun
        QVector3D sunPos = calculateSunPositionAstronomical(skyboxRadius, time);
        QVector3D moonDir = QVector3D(x, y, z).normalized();
        QVector3D sunDir = sunPos.normalized();
        
        // If moon and sun are too close, adjust the moon position
        float cosAngle = QVector3D::dotProduct(moonDir, sunDir);
        if (cosAngle > 0.7) { // Within about 45 degrees
            // Rotate moon position to be perpendicular to sun
            QVector3D crossProd = QVector3D::crossProduct(sunDir, QVector3D(0, 1, 0)).normalized();
            x = distance * crossProd.x();
            y = distance * 0.2; // Keep moon a bit above horizon
            z = distance * crossProd.z();
        }
        
        // Only log once per calculation
        static QDateTime lastMoonLogTime;
        if (lastMoonLogTime != time) {
            qDebug() << "Moon position:" << x << y << z << "Phase:" << phase;
            lastMoonLogTime = time;
        }
        
        return QVector3D(x, y, z);
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in moon position calculation:" << e.what();
        return QVector3D(0, -skyboxRadius * 0.7, 0); // Default position (below horizon)
    }
    catch (...) {
        qCritical() << "Unknown exception in moon position calculation";
        return QVector3D(0, -skyboxRadius * 0.7, 0); // Default position (below horizon)
    }
}