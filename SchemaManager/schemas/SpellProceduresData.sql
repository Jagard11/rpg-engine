-- ./SchemaManager/schemas/SpellProceduresData.sql

INSERT INTO spell_procedures (id, spell_id, trigger_type, proc_order, action_type, target_type, action_value, value_modifier, chance, created_at) VALUES (1, 3, 'on_crit', 1, 'apply_state', 'self', 'Incinerate', 2, 1.0, '2025-01-28 20:17:43');
