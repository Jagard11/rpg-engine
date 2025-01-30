-- ./SchemaManager/schemas/ClassLevelTypesStructure.sql

CREATE TABLE class_level_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    max_level INTEGER NOT NULL,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
