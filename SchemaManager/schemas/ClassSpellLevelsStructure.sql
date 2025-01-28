-- ./SchemaManager/schemas/ClassSpellLevelsStructure.sql

CREATE TABLE class_spell_levels (
    id INTEGER PRIMARY KEY,
    character_id INTEGER NOT NULL,
    class_id INTEGER NOT NULL,
    level_number INTEGER NOT NULL,
    purchase_order INTEGER NOT NULL,
    spell_selection_1 INTEGER,
    spell_selection_2 INTEGER,
    spell_selection_3 INTEGER,
    spell_selection_4 INTEGER,
    spell_selection_5 INTEGER,
    spell_selection_6 INTEGER,
    unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);
