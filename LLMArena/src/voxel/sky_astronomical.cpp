// src/voxel/sky_astronomical.cpp
#include "../../include/voxel/sky_system.h"
#include "../../include/ui/location_dialog.h"
#include <QSettings>
#include <QtMath>
#include <QDebug>

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
        double m = dateTime.time().minute();
        double s = dateTime.time().second();
        double ms = dateTime.time().msec();
        double dayFraction = (h + m/60.0 + s/3600.0 + ms/3600000.0) / 24.0;
        
        // Calculate Julian Day
        int a = (14 - M) / 12;
        int y = Y + 4800 - a;
        int m = M + 12*a - 3;
        
        double jd = D + (153*m + 2)/5 + 365*y + y/4 - y/100 + y/400 - 32045 + dayFraction;
        
        return jd;
    }
    
    // Calculate Julian Centuries since J2000.0
    double julianCentury(double jd) {
        return (jd - 2451545.0) / 36525.0;
    }
}

// Calculate sun position using declination and hour angle
QVector3D SkySystem::calculateSunPositionAstronomical(const QDateTime& time) {
    // Get the current location data
    LocationData location = LocationDialog::loadLocation();
    if (location.name.isEmpty()) {
        // Use default formula if no location is set
        return calculateSunPosition(time);
    }
    
    // Convert to UTC time
    QDateTime utcTime = time.toUTC();
    
    // Get day of year and hour
    int doy = dayOfYear(utcTime);
    double hour = fractionalHour(utcTime);
    
    // Calculate solar declination (approximate formula)
    double declination = 23.45 * qSin(toRadians(360.0/365.0 * (doy - 81)));
    
    // Calculate hour angle
    double longitude = location.longitude;
    double hourAngle = 15.0 * (hour - 12.0) + longitude;
    
    // Convert to radians
    double lat = toRadians(location.latitude);
    double decl = toRadians(declination);
    double ha = toRadians(hourAngle);
    
    // Calculate altitude and azimuth of the sun
    double sinAlt = qSin(lat) * qSin(decl) + qCos(lat) * qCos(decl) * qCos(ha);
    double altitude = qAsin(sinAlt);
    
    double cosAz = (qSin(decl) - qSin(lat) * sinAlt) / (qCos(lat) * qCos(altitude));
    double azimuth = qAcos(qBound(-1.0, cosAz, 1.0));
    
    // Convert altitude and azimuth to XYZ coordinates
    // X = distance * cos(altitude) * sin(azimuth)
    // Y = distance * sin(altitude)
    // Z = distance * cos(altitude) * cos(azimuth)
    
    if (qSin(ha) < 0) {
        azimuth = 2 * M_PI - azimuth;
    }
    
    double distance = m_skyboxRadius * 0.8;
    double x = distance * qCos(altitude) * qSin(azimuth);
    double y = distance * qSin(altitude);
    double z = -distance * qCos(altitude) * qCos(azimuth); // Negate Z for OpenGL coordinates
    
    // Debug output
    qDebug() << "Sun calculation:";
    qDebug() << "  Date/Time:" << time.toString() << "UTC:" << utcTime.toString();
    qDebug() << "  Location:" << location.name << location.latitude << location.longitude;
    qDebug() << "  DOY:" << doy << "Hour:" << hour;
    qDebug() << "  Declination:" << declination << "degrees";
    qDebug() << "  Hour Angle:" << hourAngle << "degrees";
    qDebug() << "  Altitude:" << toDegrees(altitude) << "degrees";
    qDebug() << "  Azimuth:" << toDegrees(azimuth) << "degrees";
    qDebug() << "  Position:" << x << y << z;
    
    return QVector3D(x, y, z);
}

// Calculate moon position
QVector3D SkySystem::calculateMoonPositionAstronomical(const QDateTime& time) {
    // Get the current location data
    LocationData location = LocationDialog::loadLocation();
    if (location.name.isEmpty()) {
        // Use default formula if no location is set
        return calculateMoonPosition(time);
    }
    
    // Moon position is more complex than sun position
    // Here we'll use a very simplified model based on sun position
    
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
    
    // Simplified formula for lunar declination and right ascension
    double declination = 23.45 * qSin(F); // Very simplified
    
    // Lunar phase (0-1)
    double phaseAngle = fmod(L0 - M, 2 * M_PI);
    double phase = 0.5 * (1 - qCos(phaseAngle));
    
    // Hour angle calculation similar to sun
    double hour = fractionalHour(time.toUTC());
    double longitude = location.longitude;
    double hourAngle = 15.0 * (hour - 12.0) + longitude + 180.0; // Opposite to sun
    
    // Convert to radians
    double lat = toRadians(location.latitude);
    double decl = toRadians(declination);
    double ha = toRadians(hourAngle);
    
    // Calculate altitude and azimuth of the moon
    double sinAlt = qSin(lat) * qSin(decl) + qCos(lat) * qCos(decl) * qCos(ha);
    double altitude = qAsin(sinAlt);
    
    double cosAz = (qSin(decl) - qSin(lat) * sinAlt) / (qCos(lat) * qCos(altitude));
    double azimuth = qAcos(qBound(-1.0, cosAz, 1.0));
    
    if (qSin(ha) < 0) {
        azimuth = 2 * M_PI - azimuth;
    }
    
    // Convert altitude and azimuth to XYZ coordinates
    double distance = m_skyboxRadius * 0.7;
    double x = distance * qCos(altitude) * qSin(azimuth);
    double y = distance * qSin(altitude);
    double z = -distance * qCos(altitude) * qCos(azimuth); // Negate Z for OpenGL coordinates
    
    // Ensure the moon is not directly opposite the sun (would look unnatural)
    QVector3D sunPos = calculateSunPositionAstronomical(time);
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
    
    qDebug() << "Moon position:" << x << y << z << "Phase:" << phase;
    
    return QVector3D(x, y, z);
}