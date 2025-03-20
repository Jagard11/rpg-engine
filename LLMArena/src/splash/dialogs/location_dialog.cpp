// src/ui/location_dialog.cpp
#include "../../include/ui/location_dialog.h"
#include <QGroupBox>
#include <QRadioButton>
#include <QDebug>

LocationDialog::LocationDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Set Location");
    
    // Get predefined locations
    defaultLocations = LocationData::getDefaultLocations();
    
    setupUI();
    
    // Load any previously saved location
    LocationData savedLocation = loadLocation();
    if (!savedLocation.name.isEmpty()) {
        // Check if it matches a predefined location
        int index = -1;
        for (int i = 0; i < defaultLocations.size(); ++i) {
            if (defaultLocations[i].name == savedLocation.name) {
                index = i;
                break;
            }
        }
        
        if (index >= 0) {
            locationCombo->setCurrentIndex(index);
        } else {
            // It's a custom location
            locationCombo->setCurrentIndex(locationCombo->count() - 1);
            customNameEdit->setText(savedLocation.name);
            latitudeSpin->setValue(savedLocation.latitude);
            longitudeSpin->setValue(savedLocation.longitude);
            
            // Find matching timezone
            int tzIndex = timeZoneCombo->findText(savedLocation.timeZoneId);
            if (tzIndex >= 0) {
                timeZoneCombo->setCurrentIndex(tzIndex);
            }
            
            // Enable custom fields
            customNameEdit->setEnabled(true);
            latitudeSpin->setEnabled(true);
            longitudeSpin->setEnabled(true);
            timeZoneCombo->setEnabled(true);
        }
    }
}

void LocationDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Info label
    QLabel *infoLabel = new QLabel("Select your location to accurately position the sun and moon in the game world.", this);
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);
    
    // Location selection
    QGroupBox *locationGroup = new QGroupBox("Location", this);
    QVBoxLayout *locationLayout = new QVBoxLayout(locationGroup);
    
    // Predefined locations
    QHBoxLayout *predefLayout = new QHBoxLayout();
    QLabel *predefLabel = new QLabel("Predefined:", this);
    locationCombo = new QComboBox(this);
    
    // Add predefined locations
    for (const LocationData &loc : defaultLocations) {
        locationCombo->addItem(loc.name);
    }
    
    // Add custom option
    locationCombo->addItem("Custom...");
    
    predefLayout->addWidget(predefLabel);
    predefLayout->addWidget(locationCombo, 1);
    locationLayout->addLayout(predefLayout);
    
    // Custom location
    QGroupBox *customGroup = new QGroupBox("Custom Location", this);
    QFormLayout *customLayout = new QFormLayout(customGroup);
    
    customNameEdit = new QLineEdit(this);
    latitudeSpin = new QDoubleSpinBox(this);
    latitudeSpin->setRange(-90.0, 90.0);
    latitudeSpin->setDecimals(4);
    latitudeSpin->setSingleStep(0.1);
    latitudeSpin->setSuffix("°");
    
    longitudeSpin = new QDoubleSpinBox(this);
    longitudeSpin->setRange(-180.0, 180.0);
    longitudeSpin->setDecimals(4);
    longitudeSpin->setSingleStep(0.1);
    longitudeSpin->setSuffix("°");
    
    timeZoneCombo = new QComboBox(this);
    populateTimeZones();
    
    customLayout->addRow("Name:", customNameEdit);
    customLayout->addRow("Latitude:", latitudeSpin);
    customLayout->addRow("Longitude:", longitudeSpin);
    customLayout->addRow("Time Zone:", timeZoneCombo);
    
    // Disable custom fields initially
    customNameEdit->setEnabled(false);
    latitudeSpin->setEnabled(false);
    longitudeSpin->setEnabled(false);
    timeZoneCombo->setEnabled(false);
    
    locationLayout->addWidget(customGroup);
    mainLayout->addWidget(locationGroup);
    
    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(locationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LocationDialog::onLocationSelected);
    
    setLayout(mainLayout);
    setMinimumWidth(400);
}

void LocationDialog::populateTimeZones()
{
    QList<QByteArray> timeZoneIds = QTimeZone::availableTimeZoneIds();
    
    for (const QByteArray &id : timeZoneIds) {
        timeZoneCombo->addItem(QString::fromUtf8(id));
    }
    
    // Try to select the system time zone
    QTimeZone localZone = QTimeZone::systemTimeZone();
    int index = timeZoneCombo->findText(QString::fromUtf8(localZone.id()));
    
    if (index >= 0) {
        timeZoneCombo->setCurrentIndex(index);
    }
}

void LocationDialog::onLocationSelected(int index)
{
    bool isCustom = (index == locationCombo->count() - 1);
    
    customNameEdit->setEnabled(isCustom);
    latitudeSpin->setEnabled(isCustom);
    longitudeSpin->setEnabled(isCustom);
    timeZoneCombo->setEnabled(isCustom);
    
    if (!isCustom && index >= 0 && index < defaultLocations.size()) {
        // Selected a predefined location, update fields
        const LocationData &loc = defaultLocations[index];
        
        // Update custom fields with the predefined values (but keep them disabled)
        latitudeSpin->setValue(loc.latitude);
        longitudeSpin->setValue(loc.longitude);
        
        // Find and select the matching time zone
        int tzIndex = timeZoneCombo->findText(loc.timeZoneId);
        if (tzIndex >= 0) {
            timeZoneCombo->setCurrentIndex(tzIndex);
        }
    }
}

// onCustomToggled slot removed - functionality handled in onLocationSelected

LocationData LocationDialog::getSelectedLocation() const
{
    int index = locationCombo->currentIndex();
    bool isCustom = (index == locationCombo->count() - 1);
    
    if (!isCustom && index >= 0 && index < defaultLocations.size()) {
        return defaultLocations[index];
    } else {
        // Custom location
        QString name = customNameEdit->text();
        if (name.isEmpty()) {
            name = "Custom Location";
        }
        
        return LocationData(
            name,
            latitudeSpin->value(),
            longitudeSpin->value(),
            timeZoneCombo->currentText()
        );
    }
}

void LocationDialog::saveLocation(const LocationData& location)
{
    QSettings settings("OobaboogaRPG", "ArenaApp");
    settings.setValue("location/name", location.name);
    settings.setValue("location/latitude", location.latitude);
    settings.setValue("location/longitude", location.longitude);
    settings.setValue("location/timeZone", location.timeZoneId);
}

LocationData LocationDialog::loadLocation()
{
    QSettings settings("OobaboogaRPG", "ArenaApp");
    LocationData location;
    
    location.name = settings.value("location/name", "").toString();
    location.latitude = settings.value("location/latitude", 0.0).toDouble();
    location.longitude = settings.value("location/longitude", 0.0).toDouble();
    location.timeZoneId = settings.value("location/timeZone", "").toString();
    
    return location;
}