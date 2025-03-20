// include/voxel/sky_system_helpers.h
#ifndef SKY_SYSTEM_HELPERS_H
#define SKY_SYSTEM_HELPERS_H

#include <QDateTime>
#include <QtMath>
#include <QVector3D>

namespace SkyHelpers {
    // Convert degrees to radians
    inline double toRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }
    
    // Convert radians to degrees
    inline double toDegrees(double radians) {
        return radians * 180.0 / M_PI;
    }
    
    // Calculate day of year (0-365)
    inline int dayOfYear(const QDateTime& date) {
        return date.date().dayOfYear() - 1;
    }
    
    // Calculate fractional hour (0-24)
    inline double fractionalHour(const QDateTime& date) {
        return date.time().hour() + date.time().minute() / 60.0 + date.time().second() / 3600.0;
    }
    
    // Calculate Julian Day from QDateTime
    inline double julianDay(const QDateTime& dateTime) {
        // Get date components
        QDate date = dateTime.date();
        int Y = date.year();
        int M = date.month();
        int D = date.day();
        
        // Get time components and convert to decimal day
        double h = dateTime.time().hour();
        double min = dateTime.time().minute();
        double s = dateTime.time().second();
        double ms = dateTime.time().msec();
        double dayFraction = (h + min/60.0 + s/3600.0 + ms/3600000.0) / 24.0;
        
        // Calculate Julian Day
        int a = (14 - M) / 12;
        int y = Y + 4800 - a;
        int mm = M + 12*a - 3;
        
        double jd = D + (153*mm + 2)/5 + 365*y + y/4 - y/100 + y/400 - 32045 + dayFraction;
        
        return jd;
    }
    
    // Calculate Julian Centuries since J2000.0
    inline double julianCentury(double jd) {
        return (jd - 2451545.0) / 36525.0;
    }
}

#endif // SKY_SYSTEM_HELPERS_H