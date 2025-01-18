# ./DatabaseSetup/createClassSpellSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_class_spell_schema():
    """Create the class spells table schema"""
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
        
        # Create class spells table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_spells (
            id INTEGER PRIMARY KEY,
            class_id INTEGER NOT NULL,
            spell_id INTEGER NOT NULL,
            min_level INTEGER NOT NULL DEFAULT 1,
            is_auto_granted BOOLEAN DEFAULT FALSE,
            auto_grant_order INTEGER,
            is_optional BOOLEAN DEFAULT TRUE,
            prerequisites_json JSON,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_id) REFERENCES character_classes(id),
            FOREIGN KEY (spell_id) REFERENCES spells(id)
        )
        """)
        
        # Create character spell acquisition table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_spell_acquisition (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            class_id INTEGER NOT NULL,
            spell_id INTEGER NOT NULL,
            acquired_at_level INTEGER NOT NULL,
            acquisition_order INTEGER NOT NULL,
            is_active BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (class_id) REFERENCES character_classes(id),
            FOREIGN KEY (spell_id) REFERENCES spells(id)
        )
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