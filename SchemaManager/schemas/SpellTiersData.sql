-- ./SchemaManager/schemas/SpellTiersData.sql

INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (1, 'Cantrip', 0, 'Basic magical effects requiring minimal power', 0, 17);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (2, 'Tier 1', 1, 'Foundational spells accessible to novice spellcasters', 7, 15);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (3, 'Tier 2', 2, 'Intermediate spells with increased complexity and power', 14, 14);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (4, 'Tier 3', 3, 'Advanced spells requiring significant magical prowess', 21, 13);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (5, 'Tier 4', 4, 'Expert-level spells with potent magical effects', 28, 12);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (6, 'Tier 5', 5, 'Master-level spells demonstrating exceptional control', 35, 11);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (7, 'Tier 6', 6, 'Elite spells showcasing mastery over magical forces', 42, 10);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (8, 'Tier 7', 7, 'Legendary spells with devastating potential', 49, 9);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (9, 'Tier 8', 8, 'Mythical spells of tremendous power', 56, 8);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (10, 'Tier 9', 9, 'Ancient spells of incredible magical might', 63, 7);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (11, 'Tier 10', 10, 'Ultimate spells representing pinnacle of magical achievement', 70, 6);
INSERT INTO spell_tiers (id, tier_name, tier_number, description, min_level, max_slots) VALUES (12, 'Super Tier', 11, 'Transcendent spells beyond normal magical limitations', 77, 5);
