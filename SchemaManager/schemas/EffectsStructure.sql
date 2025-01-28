-- ./SchemaManager/schemas/EffectsStructure.sql

CREATE TABLE effects (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    effect_type_id INTEGER NOT NULL,
    base_value INTEGER,
    value_scaling TEXT,
    duration INTEGER,
    tick_type TEXT,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (effect_type_id) REFERENCES effect_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);
