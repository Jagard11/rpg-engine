CREATE TABLE IF NOT EXISTS effects (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    effect_type_id INTEGER NOT NULL,
    base_value INTEGER,
    value_scaling TEXT,
    duration INTEGER,
    tick_type TEXT,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);