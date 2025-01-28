-- ./SchemaManager/schemas/SpellRequirementsStructure.sql

-- Create table for spell requirements
CREATE TABLE IF NOT EXISTS spell_requirements (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    requirement_group INTEGER NOT NULL,
    requirement_type TEXT NOT NULL,
    target_type TEXT NOT NULL,
    target_value TEXT,
    comparison_type TEXT,
    value INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create table for spell states
CREATE TABLE IF NOT EXISTS spell_states (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    max_stacks INTEGER DEFAULT 1,
    duration INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create table for character spell states
CREATE TABLE IF NOT EXISTS character_spell_states (
    id INTEGER PRIMARY KEY,
    character_id INTEGER NOT NULL,
    spell_state_id INTEGER NOT NULL,
    current_stacks INTEGER DEFAULT 1,
    remaining_duration INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create table for spell procedures
CREATE TABLE IF NOT EXISTS spell_procedures (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    trigger_type TEXT NOT NULL,
    proc_order INTEGER NOT NULL,
    action_type TEXT NOT NULL,
    target_type TEXT NOT NULL,
    action_value TEXT NOT NULL,
    value_modifier INTEGER,
    chance REAL DEFAULT 1.0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);