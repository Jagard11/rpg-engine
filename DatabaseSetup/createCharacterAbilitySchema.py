# ./DatabaseSetup/createCharacterAbilitySchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_ability_schema():
    """Create the character abilities table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating character ability schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create character abilities table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_abilities (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            ability_id INTEGER NOT NULL,
            acquired_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            current_cooldown INTEGER DEFAULT 0,
            times_used INTEGER DEFAULT 0,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (ability_id) REFERENCES abilities(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Character ability schema creation completed successfully.")
        
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
    create_character_ability_schema()

if __name__ == "__main__":
    main()