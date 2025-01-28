-- ./SchemaManager/schemas/EffectTypesData.sql

INSERT INTO effect_types (id, name, description) VALUES (1, 'damage', 'Deals damage to target');
INSERT INTO effect_types (id, name, description) VALUES (2, 'healing', 'Restores health to target');
INSERT INTO effect_types (id, name, description) VALUES (3, 'stat_modify', 'Modifies target stats');
INSERT INTO effect_types (id, name, description) VALUES (4, 'status', 'Applies a status condition');
INSERT INTO effect_types (id, name, description) VALUES (5, 'control', 'Controls target movement/actions');
INSERT INTO effect_types (id, name, description) VALUES (6, 'summon', 'Summons creatures/objects');
