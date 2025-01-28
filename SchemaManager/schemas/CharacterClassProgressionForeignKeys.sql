-- ./SchemaManager/schemas/CharacterClassProgressionForeignKeys.sql

CREATE TABLE character_class_progression (
    id INTEGER PRIMARY KEY,
    character_id INTEGER NOT NULL,
    class_id INTEGER NOT NULL,
    current_level INTEGER NOT NULL DEFAULT 0,
    current_experience INTEGER NOT NULL DEFAULT 0,
    unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);
