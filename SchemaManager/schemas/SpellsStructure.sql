CREATE TABLE IF NOT EXISTS spells (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    spell_tier INTEGER NOT NULL,
    is_super_tier BOOLEAN DEFAULT FALSE,
    mp_cost INTEGER NOT NULL DEFAULT 0,
    casting_time TEXT,
    range TEXT,
    area_of_effect TEXT,
    damage_base INTEGER,
    damage_scaling TEXT,
    healing_base INTEGER,
    healing_scaling TEXT,
    status_effects TEXT,
    duration TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);