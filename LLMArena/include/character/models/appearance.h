// include/character/models/appearance.h
#ifndef CHARACTER_MODELS_APPEARANCE_H
#define CHARACTER_MODELS_APPEARANCE_H

#include <QString>
#include <QJsonObject>

// Structure for character collision geometry
struct CharacterCollisionGeometry {
    double width;
    double height;
    double depth;
    
    CharacterCollisionGeometry() : width(1.0), height(2.0), depth(1.0) {}
    
    QJsonObject toJson() const {
        QJsonObject json;
        json["width"] = width;
        json["height"] = height;
        json["depth"] = depth;
        return json;
    }
    
    static CharacterCollisionGeometry fromJson(const QJsonObject &json) {
        CharacterCollisionGeometry geometry;
        geometry.width = json["width"].toDouble(1.0);
        geometry.height = json["height"].toDouble(2.0);
        geometry.depth = json["depth"].toDouble(1.0);
        return geometry;
    }
};

// Structure to represent a character's appearance
struct CharacterAppearance {
    QString gender;
    QString age;
    QString height;
    QString build;
    QString hairColor;
    QString hairStyle;
    QString eyeColor;
    QString skinTone;
    QString clothing;
    QString distinguishingFeatures;
    QString generalDescription;
    
    // 3D visualization properties
    QString spritePath;                   // Path to the character sprite image
    CharacterCollisionGeometry collision; // Collision geometry dimensions
    
    // Create a JSON object from this appearance
    QJsonObject toJson() const {
        QJsonObject json;
        json["gender"] = gender;
        json["age"] = age;
        json["height"] = height;
        json["build"] = build;
        json["hairColor"] = hairColor;
        json["hairStyle"] = hairStyle;
        json["eyeColor"] = eyeColor;
        json["skinTone"] = skinTone;
        json["clothing"] = clothing;
        json["distinguishingFeatures"] = distinguishingFeatures;
        json["generalDescription"] = generalDescription;
        json["spritePath"] = spritePath;
        json["collision"] = collision.toJson();
        return json;
    }
    
    // Create an appearance from a JSON object
    static CharacterAppearance fromJson(const QJsonObject &json) {
        CharacterAppearance appearance;
        appearance.gender = json["gender"].toString();
        appearance.age = json["age"].toString();
        appearance.height = json["height"].toString();
        appearance.build = json["build"].toString();
        appearance.hairColor = json["hairColor"].toString();
        appearance.hairStyle = json["hairStyle"].toString();
        appearance.eyeColor = json["eyeColor"].toString();
        appearance.skinTone = json["skinTone"].toString();
        appearance.clothing = json["clothing"].toString();
        appearance.distinguishingFeatures = json["distinguishingFeatures"].toString();
        appearance.generalDescription = json["generalDescription"].toString();
        appearance.spritePath = json["spritePath"].toString();
        
        if (json.contains("collision") && json["collision"].isObject()) {
            appearance.collision = CharacterCollisionGeometry::fromJson(json["collision"].toObject());
        }
        
        return appearance;
    }
};

#endif // CHARACTER_MODELS_APPEARANCE_H