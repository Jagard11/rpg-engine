CREATE TABLE effect_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT
);

CREATE TABLE magic_schools (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE range_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE damage_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE target_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE modification_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE trigger_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE area_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE stat_types (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    is_resource BOOLEAN DEFAULT FALSE,
    can_be_modified BOOLEAN DEFAULT TRUE,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE resources (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT
);

CREATE TABLE spell_tiers (
    id INTEGER PRIMARY KEY,
    tier_name TEXT NOT NULL,
    tier_number INTEGER NOT NULL,
    description TEXT,
    min_level INTEGER,
    max_slots INTEGER
);

CREATE TABLE spell_states (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    max_stacks INTEGER DEFAULT 1,
    duration INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE spell_effects (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    effect_type_id INTEGER NOT NULL,
    magic_school_id INTEGER NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (effect_type_id) REFERENCES effect_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (magic_school_id) REFERENCES magic_schools(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);

CREATE TABLE damage_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    range_type_id INTEGER NOT NULL,
    range_distance INTEGER,
    base_damage TEXT,
    resistance_save_id INTEGER,
    resistance_effect TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (range_type_id) REFERENCES range_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    FOREIGN KEY (resistance_save_id) REFERENCES stat_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);

CREATE TABLE buff_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    range_type_id INTEGER NOT NULL,
    range_distance INTEGER DEFAULT 0,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (range_type_id) REFERENCES range_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);

CREATE TABLE state_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    spell_state_id INTEGER NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (spell_state_id) REFERENCES spell_states(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    UNIQUE(spell_effect_id, spell_state_id)
);

CREATE TABLE transformation_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    new_model_id INTEGER,
    stat_changes TEXT,
    duration INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION
);

CREATE TABLE summoning_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    creature_id INTEGER,
    duration INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION
);

CREATE TABLE summoning_options (
    id INTEGER PRIMARY KEY,
    summoning_effect_id INTEGER NOT NULL,
    option_type TEXT NOT NULL,
    number_of_creatures INTEGER,
    creature_tier INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (summoning_effect_id) REFERENCES summoning_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION
);

CREATE TABLE area_trigger_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    shape TEXT NOT NULL,
    size INTEGER,
    applied_effect_id INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (applied_effect_id) REFERENCES spell_effects(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);

CREATE TABLE create_object_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    object_type TEXT,
    object_id INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION
);

CREATE TABLE add_item_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    item_type TEXT,
    item_id INTEGER,
    quantity INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION
);

CREATE TABLE inventory_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    action_type TEXT NOT NULL,
    item_type TEXT,
    item_subtype TEXT,
    target_item_type TEXT,
    target_item_subtype TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION
);

CREATE TABLE spell_lists (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE spells (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    description TEXT,
    spell_tier INTEGER NOT NULL,
    is_super_tier BOOLEAN DEFAULT FALSE,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_tier) REFERENCES spell_tiers(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);

CREATE TABLE spell_list_entries (
    id INTEGER PRIMARY KEY,
    spell_list_id INTEGER NOT NULL,
    spell_id INTEGER NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_list_id) REFERENCES spell_lists(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    UNIQUE(spell_list_id, spell_id)
);

CREATE TABLE random_spell_cast_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    spell_list_id INTEGER NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (spell_list_id) REFERENCES spell_lists(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);

CREATE TABLE disable_spell_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    target_spell_id INTEGER NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (target_spell_id) REFERENCES spells(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);

CREATE TABLE duration_modifier_effects (
    id INTEGER PRIMARY KEY,
    spell_effect_id INTEGER NOT NULL,
    modifier_type TEXT NOT NULL,
    value REAL,
    target_effect_type TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE CASCADE ON UPDATE NO ACTION
);

CREATE TABLE spell_costs (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    resource_id INTEGER NOT NULL,
    cost_amount INTEGER NOT NULL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (resource_id) REFERENCES resources(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    UNIQUE(spell_id, resource_id)
);

CREATE TABLE spell_has_effects (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    spell_effect_id INTEGER NOT NULL,
    effect_order INTEGER NOT NULL DEFAULT 1,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (spell_effect_id) REFERENCES spell_effects(id) ON DELETE NO ACTION ON UPDATE NO ACTION,
    UNIQUE(spell_id, spell_effect_id)
);

CREATE TABLE spell_requirements (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    requirement_type TEXT NOT NULL,
    target_type TEXT NOT NULL,
    target_value TEXT,
    comparison_type TEXT,
    value INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION
);

CREATE TABLE spell_targeting (
    id INTEGER PRIMARY KEY,
    spell_id INTEGER NOT NULL,
    max_targets INTEGER NOT NULL DEFAULT 1,
    requires_los BOOLEAN NOT NULL DEFAULT TRUE,
    allow_dead_targets BOOLEAN NOT NULL DEFAULT FALSE,
    ignore_target_immunity BOOLEAN NOT NULL DEFAULT FALSE,
    min_range INTEGER NOT NULL DEFAULT 0,
    max_range INTEGER NOT NULL DEFAULT 30,
    area_type_id INTEGER,
    area_size INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (area_type_id) REFERENCES area_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);

CREATE TABLE character_spell_states (
    id INTEGER PRIMARY KEY,
    character_id INTEGER NOT NULL,
    spell_state_id INTEGER NOT NULL,
    current_stacks INTEGER DEFAULT 1,
    remaining_duration INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE ON UPDATE NO ACTION,
    FOREIGN KEY (spell_state_id) REFERENCES spell_states(id) ON DELETE NO ACTION ON UPDATE NO ACTION
);