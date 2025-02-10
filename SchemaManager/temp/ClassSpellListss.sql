-- ./SchemaManager/imports/ClassSpellLists.sql

-- Table for associating spells with class spell lists
CREATE TABLE class_spell_lists (
    id INTEGER PRIMARY KEY,
    class_id INTEGER NOT NULL,
    spell_id INTEGER NOT NULL,
    list_name TEXT NOT NULL,
    minimum_level INTEGER NOT NULL DEFAULT 1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    UNIQUE(class_id, spell_id) -- Prevent duplicate spells in the same class list
);
