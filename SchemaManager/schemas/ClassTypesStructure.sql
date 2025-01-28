-- ./SchemaManager/schemas/ClassTypesStructure.sql

CREATE TABLE class_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
