CREATE TABLE IF NOT EXISTS characters (
    id INTEGER PRIMARY KEY,
    first_name TEXT NOT NULL,
    middle_name TEXT,
    last_name TEXT,
    bio TEXT,
    total_level INTEGER NOT NULL DEFAULT 0,
    birth_place TEXT,
    age INTEGER,
    karma INTEGER DEFAULT 0,
    talent TEXT,
    race_category_id INTEGER NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);