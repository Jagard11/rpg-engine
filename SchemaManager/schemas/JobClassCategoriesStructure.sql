-- ./SchemaManager/schemas/JobClassCategoriesStructure.sql

CREATE TABLE job_class_categories (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

