-- ./SchemaManager/schemas/SpellStatesStructure.sql

CREATE TABLE spell_states (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    max_stacks INTEGER DEFAULT 1,
    duration INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
