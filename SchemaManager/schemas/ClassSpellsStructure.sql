-- ./SchemaManager/schemas/ClassSpellsStructure.sql

CREATE TABLE class_spells (
    id INTEGER PRIMARY KEY,
    job_class_id INTEGER NOT NULL,
    spell_name TEXT NOT NULL,
    spell_tier INTEGER NOT NULL, -- 0-10 for normal tiers, 11 for super tier
    mp_cost INTEGER NOT NULL,
    description TEXT,
    requirements TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (job_class_id) REFERENCES job_classes(id)
);

