-- ./SchemaManager/schemas/ClassCategoriesStructure.sql

CREATE TABLE class_categories (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    is_racial BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
