ALTER TABLE class_spell_levels ADD CONSTRAINT fk_class_spell_levels_class_id_classes FOREIGN KEY (class_id) REFERENCES classes(id) ON DELETE NO ACTION ON UPDATE NO ACTION;
ALTER TABLE class_spell_levels ADD CONSTRAINT fk_class_spell_levels_character_id_characters FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE NO ACTION ON UPDATE NO ACTION;