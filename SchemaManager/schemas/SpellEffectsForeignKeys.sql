-- ./SchemaManager/schemas/SpellEffectsForeignKeys.sql

CREATE TABLE spell_effects (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    effect_id INTEGER NOT NULL,
    effect_order INTEGER NOT NULL,
    probability REAL DEFAULT 1.0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (effect_id) REFERENCES effects(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION
);
