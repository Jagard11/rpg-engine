// include/ui/location_dialog.h
#ifndef LOCATION_DIALOG_H
#define LOCATION_DIALOG_H

#include <QDialog>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QSettings>
#include <QDateTime>
#include <QTimeZone>

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

class LocationDialog : public QDialog
{
    Q_OBJECT
    
public:
    LocationDialog(QWidget *parent = nullptr);
    
    LocationData getSelectedLocation() const;
    
    static void saveLocation(const LocationData& location);
    static LocationData loadLocation();
    
private:
    QComboBox *locationCombo;
    QLineEdit *customNameEdit;
    QDoubleSpinBox *latitudeSpin;
    QDoubleSpinBox *longitudeSpin;
    QComboBox *timeZoneCombo;
    
    QList<LocationData> defaultLocations;
    
    void setupUI();
    void populateTimeZones();
    
private slots:
    void onLocationSelected(int index);
};

#endif // LOCATION_DIALOG_H