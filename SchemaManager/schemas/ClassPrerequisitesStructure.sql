CREATE TABLE IF NOT EXISTS class_prerequisites (
    id INTEGER PRIMARY KEY,
    class_id INTEGER NOT NULL,
    prerequisite_group INTEGER NOT NULL,
    prerequisite_type TEXT NOT NULL,
    target_id INTEGER,
    required_level INTEGER,
    min_value INTEGER,
    max_value INTEGER
);