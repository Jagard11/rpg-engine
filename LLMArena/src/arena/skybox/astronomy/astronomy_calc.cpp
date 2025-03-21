// src/arena/skybox/astronomy/astronomy_calc.cpp
#include "../../include/arena/skybox/sky_system_helpers.h"
#include <QVector3D>
#include <QSettings>
#include <QDebug>
#include <QTimeZone>

// Forward declarations for simple position calculation functions
QVector3D calculateSunPositionSimple(float skyboxRadius, const QDateTime& time);
QVector3D calculateMoonPositionSimple(float skyboxRadius, const QDateTime& time);

// Forward declaration for LocationData
class LocationData {
public:
    QString name;
    double latitude;
    double longitude;
    QString timeZoneId;
    
    LocationData() : latitude(0.0), longitude(0.0) {}
    
    LocationData(const QString& n, double lat, double lng, const QString& tz) 
        : name(n), latitude(lat), longitude(lng), timeZoneId(tz) {}
        
    static QList<LocationData> getDefaultLocations() {
        QList<LocationData> locations;
        
        locations.append(LocationData("New York", 40.7128, -74.0060, "America/New_York"));
        locations.append(LocationData("Los Angeles", 34.0522, -118.2437, "America/Los_Angeles"));
        locations.append(LocationData("London", 51.5074, -0.1278, "Europe/London"));
        locations.append(LocationData("Tokyo", 35.6762, 139.6503, "Asia/Tokyo"));
        locations.append(LocationData("Sydney", -33.8688, 151.2093, "Australia/Sydney"));
        locations.append(LocationData("Cairo", 30.0444, 31.2357, "Africa/Cairo"));
        locations.append(LocationData("Rio de Janeiro", -22.9068, -43.1729, "America/Sao_Paulo"));
        locations.append(LocationData("Moscow", 55.7558, 37.6173, "Europe/Moscow"));
        locations.append(LocationData("Beijing", 39.9042, 116.4074, "Asia/Shanghai"));
        locations.append(LocationData("Cape Town", -33.9249, 18.4241, "Africa/Johannesburg"));
        
        return locations;
    }
};

namespace {
    // Load location from settings
    LocationData loadLocation() {
        QSettings settings("OobaboogaRPG", "ArenaApp");
        LocationData location;
        
        location.name = settings.value("location/name", "").toString();
        location.latitude = settings.value("location/latitude", 0.0).toDouble();
        location.longitude = settings.value("location/longitude", 0.0).toDouble();
        location.timeZoneId = settings.value("location/timeZone", "").toString();
        
        return location;
    }
    
    // Convert local time to solar time
    QDateTime convertToSolarTime(const QDateTime& localTime, double longitude) {
        // Get the difference in hours from Greenwich
        double hourDiff = longitude / 15.0;
        
        // Create a copy of the time adjusted for solar time
        QDateTime solarTime = localTime;
        solarTime = solarTime.addSecs(-static_cast<int>(hourDiff * 3600));
        
        return solarTime;
    }
}

// Calculate sun position using declination and hour angle
QVector3D calculateSunPositionAstronomical(float skyboxRadius, const QDateTime& time) {
    // Get the current location data
    LocationData location = loadLocation();
    if (location.name.isEmpty()) {
        // Use default formula if no location is set
        return calculateSunPositionSimple(skyboxRadius, time);
    }
    
    // Convert to local solar time based on longitude
    // This is critical - we need solar time, not standard time zones
    QDateTime solarTime = time;
    
    // Get day of year and solar time hour
    int doy = SkyHelpers::dayOfYear(solarTime);
    
    // Calculate solar declination (approximate formula)
    double declination = 23.45 * qSin(SkyHelpers::toRadians(360.0/365.0 * (doy - 81)));
    
    // Calculate hour angle (15 degrees per hour from solar noon)
    double hour = SkyHelpers::fractionalHour(solarTime);
    double hourAngle = 15.0 * (hour - 12.0);
    
    // Convert to radians
    double lat = SkyHelpers::toRadians(location.latitude);
    double decl = SkyHelpers::toRadians(declination);
    double ha = SkyHelpers::toRadians(hourAngle);
    
    // Calculate altitude and azimuth of the sun
    double sinAlt = qSin(lat) * qSin(decl) + qCos(lat) * qCos(decl) * qCos(ha);
    double altitude = qAsin(sinAlt);
    
    double cosAz = (qSin(decl) - qSin(lat) * sinAlt) / (qCos(lat) * qCos(altitude));
    double azimuth = qAcos(qBound(-1.0, cosAz, 1.0));
    
    // Adjust azimuth for the correct quadrant
    if (qSin(ha) > 0) {
        azimuth = 2 * M_PI - azimuth;
    }
    
    // Convert altitude and azimuth to XYZ coordinates
    // X = distance * cos(altitude) * sin(azimuth)
    // Y = distance * sin(altitude)
    // Z = distance * cos(altitude) * cos(azimuth)
    
    double distance = skyboxRadius * 0.8;
    double x = distance * qCos(altitude) * qSin(azimuth);
    double y = distance * qSin(altitude);
    double z = -distance * qCos(altitude) * qCos(azimuth); // Negate Z for OpenGL coordinates
    
    // Debug output
    qDebug() << "Sun calculation:";
    qDebug() << "  Date/Time:" << time.toString() << "UTC:" << time.toUTC().toString();
    qDebug() << "  Location:" << location.name << location.latitude << location.longitude;
    qDebug() << "  DOY:" << doy << "Hour:" << hour;
    qDebug() << "  Declination:" << declination << "degrees";
    qDebug() << "  Hour Angle:" << hourAngle << "degrees";
    qDebug() << "  Altitude:" << SkyHelpers::toDegrees(altitude) << "degrees";
    qDebug() << "  Azimuth:" << SkyHelpers::toDegrees(azimuth) << "degrees";
    qDebug() << "  Position:" << x << y << z;
    
    return QVector3D(x, y, z);
}

// Calculate moon position
QVector3D calculateMoonPositionAstronomical(float skyboxRadius, const QDateTime& time) {
    // Get the current location data
    LocationData location = loadLocation();
    if (location.name.isEmpty()) {
        // Use default formula if no location is set
        return calculateMoonPositionSimple(skyboxRadius, time);
    }
    
    // Moon position is more complex than sun position
    // Here we'll use a very simplified model based on sun position
    
    // Calculate Julian Day
    double jd = SkyHelpers::julianDay(time);
    double T = SkyHelpers::julianCentury(jd);
    
    // Mean orbital elements for the Moon
    double L0 = 218.316 + 481267.8813 * T;  // Mean longitude
    double M = 134.963 + 477198.8676 * T;   // Mean anomaly
    double F = 93.272 + 483202.0175 * T;    // Argument of latitude
    
    // Convert to radians and normalize
    L0 = SkyHelpers::toRadians(fmod(L0, 360.0));
    M = SkyHelpers::toRadians(fmod(M, 360.0));
    F = SkyHelpers::toRadians(fmod(F, 360.0));
    
    // Simplified formula for lunar declination and right ascension
    double declination = 23.45 * qSin(F); // Very simplified
    
    // Lunar phase (0-1)
    double phaseAngle = fmod(L0 - M, 2 * M_PI);
    double phase = 0.5 * (1 - qCos(phaseAngle));
    
    // Convert to local solar time based on longitude
    QDateTime solarTime = time;
    
    // Get hour in solar time
    double hour = SkyHelpers::fractionalHour(solarTime);
    
    // Hour angle calculation (moon is roughly opposite to sun)
    double hourAngle = 15.0 * (hour - 12.0) + 180.0; // Opposite to sun
    
    // Convert to radians
    double lat = SkyHelpers::toRadians(location.latitude);
    double decl = SkyHelpers::toRadians(declination);
    double ha = SkyHelpers::toRadians(hourAngle);
    
    // Calculate altitude and azimuth of the moon
    double sinAlt = qSin(lat) * qSin(decl) + qCos(lat) * qCos(decl) * qCos(ha);
    double altitude = qAsin(sinAlt);
    
    double cosAz = (qSin(decl) - qSin(lat) * sinAlt) / (qCos(lat) * qCos(altitude));
    double azimuth = qAcos(qBound(-1.0, cosAz, 1.0));
    
    if (qSin(ha) > 0) {
        azimuth = 2 * M_PI - azimuth;
    }
    
    // Convert altitude and azimuth to XYZ coordinates
    double distance = skyboxRadius * 0.7;
    double x = distance * qCos(altitude) * qSin(azimuth);
    double y = distance * qSin(altitude);
    double z = -distance * qCos(altitude) * qCos(azimuth); // Negate Z for OpenGL coordinates
    
    // Ensure the moon is not directly opposite the sun (would look unnatural)
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
    
    qDebug() << "Moon position:" << x << y << z << "Phase:" << phase;
    
    return QVector3D(x, y, z);
}