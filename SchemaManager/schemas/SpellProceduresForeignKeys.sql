-- ./SchemaManager/schemas/SpellProceduresForeignKeys.sql

CREATE TABLE spell_procedures (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    trigger_type TEXT NOT NULL,
    proc_order INTEGER NOT NULL,
    action_type TEXT NOT NULL,
    target_type TEXT NOT NULL,
    action_value TEXT NOT NULL,
    value_modifier INTEGER,
    chance REAL DEFAULT 1.0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION
);
