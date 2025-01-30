-- ./SchemaManager/schemas/ClassRequirementsStructure.sql

CREATE TABLE class_requirements (
    id INTEGER PRIMARY KEY,
    job_class_id INTEGER NOT NULL,
    requirement_type TEXT NOT NULL, -- 'level', 'class', 'race', 'karma', 'quest', 'kill_count'
    target_id INTEGER, -- References specific class/race/quest ID
    value INTEGER, -- Required level/count/karma value
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (job_class_id) REFERENCES job_classes(id)
);

