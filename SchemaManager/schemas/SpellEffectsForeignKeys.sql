ALTER TABLE spell_effects ADD CONSTRAINT fk_spell_effects_effect_id_effects FOREIGN KEY (effect_id) REFERENCES effects(id) ON DELETE NO ACTION ON UPDATE NO ACTION;
ALTER TABLE spell_effects ADD CONSTRAINT fk_spell_effects_spell_id_spells FOREIGN KEY (spell_id) REFERENCES spells(id) ON DELETE CASCADE ON UPDATE NO ACTION;