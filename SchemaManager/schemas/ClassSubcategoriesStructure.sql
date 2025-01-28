-- ./SchemaManager/schemas/ClassSubcategoriesStructure.sql

CREATE TABLE class_subcategories (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
