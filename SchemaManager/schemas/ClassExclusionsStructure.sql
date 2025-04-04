-- ./SchemaManager/schemas/ClassExclusionsStructure.sql

CREATE TABLE class_exclusions (
    id INTEGER PRIMARY KEY,
    class_id INTEGER NOT NULL,
    exclusion_type TEXT NOT NULL,
    target_id INTEGER,
    min_value INTEGER,
    max_value INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);
