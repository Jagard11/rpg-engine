-- ./SchemaManager/schemas/CharacterStatsStructure.sql

CREATE TABLE character_stats (
    id INTEGER PRIMARY KEY,
    character_id INTEGER NOT NULL,
    base_hp INTEGER NOT NULL DEFAULT 0,
    current_hp INTEGER NOT NULL DEFAULT 0,
    base_mp INTEGER NOT NULL DEFAULT 0,
    current_mp INTEGER NOT NULL DEFAULT 0,
    base_physical_attack INTEGER NOT NULL DEFAULT 0,
    current_physical_attack INTEGER NOT NULL DEFAULT 0,
    base_physical_defense INTEGER NOT NULL DEFAULT 0,
    current_physical_defense INTEGER NOT NULL DEFAULT 0,
    base_agility INTEGER NOT NULL DEFAULT 0,
    current_agility INTEGER NOT NULL DEFAULT 0,
    base_magical_attack INTEGER NOT NULL DEFAULT 0,
    current_magical_attack INTEGER NOT NULL DEFAULT 0,
    base_magical_defense INTEGER NOT NULL DEFAULT 0,
    current_magical_defense INTEGER NOT NULL DEFAULT 0,
    base_resistance INTEGER NOT NULL DEFAULT 0,
    current_resistance INTEGER NOT NULL DEFAULT 0,
    base_special INTEGER NOT NULL DEFAULT 0,
    current_special INTEGER NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);
