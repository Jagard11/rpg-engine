-- ./SchemaManager/schemas/ClassRequirementsData.sql

-- Example requirements for various classes
INSERT INTO class_requirements (job_class_id, requirement_type, target_id, value) VALUES
-- Champion requirements
(2, 'class', 3, 15), -- Requires level 15 Fighter

-- Paladin requirements
(5, 'class', 4, 10), -- Requires level 10 Knight
(5, 'class', 12, 5), -- Requires level 5 Cleric

-- Ninja requirements
(8, 'level', NULL, 60), -- Requires total level 60

-- World Disaster requirements
(15, 'level', NULL, 95), -- Requires total level 95

-- World Champion special conditions
(22, 'special', NULL, 1); -- Special tournament victory requirement