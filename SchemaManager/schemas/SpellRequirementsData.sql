-- ./SchemaManager/schemas/SpellRequirementsData.sql

INSERT INTO spell_requirements (id, spell_id, requirement_group, requirement_type, target_type, target_value, comparison_type, value, created_at) VALUES (1, 1, 1, 'target_status', 'target', 'immobilized', 'has_status', NULL, '2025-01-28 20:17:43');
INSERT INTO spell_requirements (id, spell_id, requirement_group, requirement_type, target_type, target_value, comparison_type, value, created_at) VALUES (2, 1, 1, 'range', 'target', 'distance', 'less_than', 1, '2025-01-28 20:17:43');
INSERT INTO spell_requirements (id, spell_id, requirement_group, requirement_type, target_type, target_value, comparison_type, value, created_at) VALUES (3, 2, 1, 'spell_state', 'self', 'Incinerate', 'greater_than', 0, '2025-01-28 20:17:43');
