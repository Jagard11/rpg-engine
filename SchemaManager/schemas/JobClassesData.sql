-- ./SchemaManager/schemas/JobClassesData.sql

-- Warrior Classes
INSERT INTO job_classes (name, category_id, level_type_id, description, prerequisites) VALUES
('Archer', 1, 1, 'Specializes in ranged combat with bows', NULL),
('Champion', 1, 2, 'Elite warrior class focused on martial combat', 'Level 15 Fighter'),
('Fighter', 1, 1, 'Basic combat specialist', NULL),
('Knight', 1, 1, 'Mounted combat specialist', NULL),
('Paladin', 1, 2, 'Holy warrior combining combat and divine power', 'Level 10 Knight, Level 5 Cleric'),
('Sword Master', 1, 2, 'Advanced sword fighting specialist', 'Level 15 Fighter');

-- Thief Classes
INSERT INTO job_classes (name, category_id, level_type_id, description, prerequisites) VALUES
('Assassin', 2, 2, 'Stealth killer specialist', 'Level 10 Rogue'),
('Ninja', 2, 2, 'Eastern stealth warrior', 'Level 60 total character level'),
('Ranger', 2, 1, 'Wilderness and tracking specialist', NULL),
('Rogue', 2, 1, 'Basic stealth and thievery specialist', NULL);

-- Magic Classes
INSERT INTO job_classes (name, category_id, level_type_id, description, prerequisites) VALUES
('Wizard', 3, 1, 'Basic arcane spellcaster', NULL),
('Cleric', 3, 1, 'Divine magic and healing specialist', NULL),
('Druid', 3, 1, 'Nature-based spellcaster', NULL),
('Necromancer', 3, 2, 'Death magic specialist', 'Level 10 Wizard'),
('World Disaster', 3, 3, 'Ultimate destructive magic user', 'Level 95 total character level');

-- Production Classes
INSERT INTO job_classes (name, category_id, level_type_id, description, prerequisites) VALUES
('Blacksmith', 4, 1, 'Metal crafting specialist', NULL),
('Cook', 4, 1, 'Food preparation specialist', NULL),
('Craftsman', 4, 1, 'General item creation specialist', NULL),
('Runesmith', 4, 2, 'Magical item crafter', 'Level 10 Blacksmith');

-- Commander Classes
INSERT INTO job_classes (name, category_id, level_type_id, description, prerequisites) VALUES
('Commander', 5, 2, 'Military leadership specialist', 'Level 10 in any warrior class'),
('Evangelist', 5, 2, 'Influential leader with mind-affecting abilities', NULL),
('General', 5, 3, 'Supreme military commander', 'Level 10 Commander');

-- Special Classes
INSERT INTO job_classes (name, category_id, level_type_id, description, special_conditions) VALUES
('World Champion', 6, 3, 'Strongest warrior class', 'Win World Champion Tournament');

-- Common Classes
INSERT INTO job_classes (name, category_id, level_type_id, description, is_genius_variant) VALUES
('Genius Fighter', 1, 1, 'Naturally talented fighter', TRUE),
('Genius Alchemist', 4, 1, 'Naturally talented alchemist', TRUE),
('Genius Paladin', 1, 2, 'Naturally talented paladin', TRUE);