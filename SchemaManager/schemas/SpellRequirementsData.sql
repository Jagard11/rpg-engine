-- ./SchemaManager/schemas/SpellRequirementsData.sql

-- Sample spell states
INSERT INTO spell_states (id, name, description, max_stacks, duration) VALUES 
(1, 'Incinerate', 'Allows casting of Phoenix Flame', 5, NULL);

-- Sample spell requirements for Pickpocket
INSERT INTO spell_requirements (spell_id, requirement_group, requirement_type, target_type, target_value, comparison_type, value) VALUES 
(1, 1, 'target_status', 'target', 'immobilized', 'has_status', NULL),
(1, 1, 'range', 'target', 'distance', 'less_than', 1);

-- Sample spell requirements for Phoenix Flame
INSERT INTO spell_requirements (spell_id, requirement_group, requirement_type, target_type, target_value, comparison_type, value) VALUES 
(2, 1, 'spell_state', 'self', 'Incinerate', 'greater_than', 0);

-- Sample spell procedures for Fireball
INSERT INTO spell_procedures (spell_id, trigger_type, proc_order, action_type, target_type, action_value, value_modifier, chance) VALUES 
(3, 'on_crit', 1, 'apply_state', 'self', 'Incinerate', 2, 1.0);