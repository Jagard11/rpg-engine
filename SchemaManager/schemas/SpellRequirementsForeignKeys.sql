-- ./SchemaManager/schemas/SpellRequirementsForeignKeys.sql

-- Foreign keys for spell requirements
ALTER TABLE spell_requirements 
ADD CONSTRAINT fk_spell_requirements_spell_id_spells 
FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION;

-- Foreign keys for character spell states
ALTER TABLE character_spell_states 
ADD CONSTRAINT fk_character_spell_states_character_id_characters 
FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE ON UPDATE NO ACTION;

ALTER TABLE character_spell_states 
ADD CONSTRAINT fk_character_spell_states_spell_state_id_spell_states 
FOREIGN KEY (spell_state_id) REFERENCES spell_states(id) ON DELETE NO ACTION ON UPDATE NO ACTION;

-- Foreign keys for spell procedures
ALTER TABLE spell_procedures 
ADD CONSTRAINT fk_spell_procedures_spell_id_spells 
FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION;