-- ./SchemaManager/schemas/ClassSpellsData.sql

-- Example spells for different classes
INSERT INTO class_spells (job_class_id, spell_name, spell_tier, mp_cost, description) VALUES
-- Wizard spells
(11, 'Magic Arrow', 0, 1, 'Basic magic missile attack'),
(11, 'Fireball', 3, 15, 'Explosive fire damage in area'),
(11, 'Meteor Fall', 9, 50, 'Devastating aerial attack'),

-- Cleric spells
(12, 'Heal Light Wounds', 0, 3, 'Basic healing spell'),
(12, 'Mass Heal', 5, 25, 'Group healing spell'),
(12, 'Resurrection', 9, 100, 'Restore life to fallen ally'),

-- Necromancer spells
(14, 'Create Undead', 3, 20, 'Create basic undead minion'),
(14, 'Death', 9, 100, 'Instant death spell');