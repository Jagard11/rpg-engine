-- ./SchemaManager/schemas/SpellTiersStructure.sql

CREATE TABLE spell_tiers (
    id INTEGER NOT NULL PRIMARY KEY,
    tier_name TEXT NOT NULL,
    tier_number INTEGER NOT NULL,
    description TEXT,
    min_level INTEGER,
    max_slots INTEGER
);
