-- ./SchemaManager/schemas/SpellRequirementsStructure.sql

CREATE TABLE spell_requirements (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    requirement_group INTEGER NOT NULL,
    requirement_type TEXT NOT NULL,
    target_type TEXT NOT NULL,
    target_value TEXT,
    comparison_type TEXT,
    value INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION
);
