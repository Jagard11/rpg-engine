# ./DatabaseSetup/createClassSpellSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_class_spell_schema():  # This name matches what initializeSchema.py expects
    """Create the class spell level table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating class spell schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create class spell levels table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_spell_levels (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            class_id INTEGER NOT NULL,
            level_number INTEGER NOT NULL,
            purchase_order INTEGER NOT NULL,
            spell_selection_1 INTEGER,
            spell_selection_2 INTEGER,
            spell_selection_3 INTEGER,
            spell_selection_4 INTEGER,
            spell_selection_5 INTEGER,
            spell_selection_6 INTEGER,
            unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (class_id) REFERENCES classes(id)
        )
        """)
        
        # Create index for faster lookups
        cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_class_spell_levels_char_class 
        ON class_spell_levels(character_id, class_id)
        """)
        
        # Create index for spell tracking
        cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_class_spell_levels_purchase 
        ON class_spell_levels(character_id, class_id, purchase_order)
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Class spell schema creation completed successfully.")
        
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
    create_class_spell_schema()

if __name__ == "__main__":
    main()