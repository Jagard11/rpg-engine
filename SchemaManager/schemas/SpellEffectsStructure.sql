CREATE TABLE IF NOT EXISTS spell_effects (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    effect_id INTEGER NOT NULL,
    effect_order INTEGER NOT NULL,
    probability REAL DEFAULT 1.0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);