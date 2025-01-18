# ./DatabaseSetup/createAutoSpellSelectionSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_auto_spell_selection_schema():
    """Create the auto spell selection mode tables schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating auto spell selection schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create auto spell selection modes table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS auto_spell_selection_modes (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            selection_strategy TEXT NOT NULL CHECK(selection_strategy IN ('Random', 'Ordered', 'Hybrid', 'Weighted', 'Custom')),
            custom_strategy_handler TEXT,
            config_template JSON,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        
        # Create class auto spell selection configs table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_auto_spell_configs (
            id INTEGER PRIMARY KEY,
            class_id INTEGER NOT NULL,
            mode_id INTEGER NOT NULL,
            config_data JSON NOT NULL,
            spell_weights JSON,
            spell_order JSON,
            custom_config JSON,
            is_active BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_id) REFERENCES character_classes(id),
            FOREIGN KEY (mode_id) REFERENCES auto_spell_selection_modes(id)
        )
        """)
        
        # Create preset spell selections table for ordered/hybrid modes
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS preset_spell_selections (
            id INTEGER PRIMARY KEY,
            class_id INTEGER NOT NULL,
            config_id INTEGER NOT NULL,
            level_threshold INTEGER NOT NULL,
            spell_id INTEGER NOT NULL,
            selection_order INTEGER NOT NULL,
            fallback_spell_ids JSON,
            conditions_json JSON,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_id) REFERENCES character_classes(id),
            FOREIGN KEY (config_id) REFERENCES class_auto_spell_configs(id),
            FOREIGN KEY (spell_id) REFERENCES spells(id)
        )
        """)
        
        # Insert default selection modes
        cursor.executemany("""
        INSERT OR IGNORE INTO auto_spell_selection_modes 
        (name, description, selection_strategy, config_template) VALUES (?, ?, ?, ?)
        """, [
            ('Pure Random', 
             'Completely random selection from available spells', 
             'Random',
             '{"excluded_spell_ids": []}'),
            
            ('Level-Ordered', 
             'Follows a predetermined order based on level', 
             'Ordered',
             '{"spell_order": [], "fallback_behavior": "random"}'),
            
            ('Weighted Random', 
             'Random selection with configurable weights per spell', 
             'Weighted',
             '{"spell_weights": {}, "default_weight": 1}'),
            
            ('Hybrid Ordered-Random', 
             'Follows order for key spells, random for others', 
             'Hybrid',
             '{"ordered_spells": [], "random_weight": 1, "ordered_weight": 2}'),
            
            ('Custom Strategy', 
             'Uses custom selection logic defined in handler', 
             'Custom',
             '{"strategy_name": "", "params": {}}')
        ])
        
        # Commit changes
        conn.commit()
        logger.info("Auto spell selection schema creation completed successfully.")
        
    except sqlite3.Error as e:
        logger.error(f"SQLite error occurred: {str(e)}")
        raise
    except Exception as e:
        logger.error(f"An error occurred: {str(e)}")
        raise
    finally:
        if conn:
            conn.close()
            logger.info("Database connection closed.")

def main():
    """Entry point for schema creation"""
    create_auto_spell_selection_schema()

if __name__ == "__main__":
    main()