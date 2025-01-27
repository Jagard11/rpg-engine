ALTER TABLE classes ADD CONSTRAINT fk_classes_subcategory_id_class_subcategories FOREIGN KEY (subcategory_id) REFERENCES class_subcategories(id) ON DELETE NO ACTION ON UPDATE NO ACTION;
ALTER TABLE classes ADD CONSTRAINT fk_classes_category_id_class_categories FOREIGN KEY (category_id) REFERENCES class_categories(id) ON DELETE NO ACTION ON UPDATE NO ACTION;
ALTER TABLE classes ADD CONSTRAINT fk_classes_class_type_class_types FOREIGN KEY (class_type) REFERENCES class_types(id) ON DELETE NO ACTION ON UPDATE NO ACTION;