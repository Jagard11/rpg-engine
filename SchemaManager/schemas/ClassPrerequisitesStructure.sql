-- ./SchemaManager/schemas/ClassPrerequisitesStructure.sql

CREATE TABLE class_prerequisites (
    id INTEGER PRIMARY KEY,
    class_id INTEGER NOT NULL,
    prerequisite_group INTEGER NOT NULL,
    prerequisite_type TEXT NOT NULL,
    target_id INTEGER,
    required_level INTEGER,
    min_value INTEGER,
    max_value INTEGER
,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);
