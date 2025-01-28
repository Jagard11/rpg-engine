-- ./SchemaManager/schemas/CharacterSpellStatesStructure.sql

CREATE TABLE character_spell_states (
    id INTEGER PRIMARY KEY,
    character_id INTEGER NOT NULL,
    spell_state_id INTEGER NOT NULL,
    current_stacks INTEGER DEFAULT 1,
    remaining_duration INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (spell_state_id) REFERENCES spell_states(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);
