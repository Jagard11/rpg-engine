CREATE TABLE IF NOT EXISTS character_class_progression (
    id INTEGER PRIMARY KEY,
    character_id INTEGER NOT NULL,
    class_id INTEGER NOT NULL,
    current_level INTEGER NOT NULL DEFAULT 0,
    current_experience INTEGER NOT NULL DEFAULT 0,
    unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);