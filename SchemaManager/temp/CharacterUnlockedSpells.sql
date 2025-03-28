-- ./SchemaManager/imports/CharacterUnlockedSpells.sql

-- Table for tracking which spells a character has unlocked at each class level
CREATE TABLE character_unlocked_spells (
    id INTEGER PRIMARY KEY,
    character_id INTEGER NOT NULL,
    class_id INTEGER NOT NULL,
    spell_id INTEGER NOT NULL,
    unlocked_at_level INTEGER NOT NULL,
    selection_order INTEGER NOT NULL, -- 1, 2, or 3 for which selection it was at that level
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    UNIQUE(character_id, class_id, spell_id), -- Character can't unlock same spell twice
    UNIQUE(character_id, class_id, unlocked_at_level, selection_order) -- Can't have two spells as same selection at same level
);