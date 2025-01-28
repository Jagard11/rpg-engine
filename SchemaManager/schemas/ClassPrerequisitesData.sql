-- ./SchemaManager/schemas/ClassPrerequisitesData.sql

INSERT INTO class_prerequisites (id, class_id, prerequisite_group, prerequisite_type, target_id, required_level, min_value, max_value) VALUES (1, 8, 0, 'specific_race', 3, NULL, NULL, NULL);
INSERT INTO class_prerequisites (id, class_id, prerequisite_group, prerequisite_type, target_id, required_level, min_value, max_value) VALUES (2, 17, 1, 'specific_race', 16, 10, NULL, NULL);
INSERT INTO class_prerequisites (id, class_id, prerequisite_group, prerequisite_type, target_id, required_level, min_value, max_value) VALUES (3, 12, 1, 'specific_race', 3, 0, NULL, NULL);
