-- ./SchemaManager/schemas/JobClassesStructure.sql

CREATE TABLE job_classes (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    category_id INTEGER NOT NULL,
    level_type_id INTEGER NOT NULL,
    description TEXT,
    prerequisites TEXT,
    special_conditions TEXT,
    is_genius_variant BOOLEAN DEFAULT FALSE,
    is_temporary BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (category_id) REFERENCES job_class_categories(id),
    FOREIGN KEY (level_type_id) REFERENCES class_level_types(id)
);

