# ./DatabaseSetup/createCharacterClassSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_class_schema():
    """Create the character classes table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating character class schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create character classes table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_classes (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            type TEXT CHECK(type IN ('Base', 'High', 'Rare')) NOT NULL,
            description TEXT,
            max_level INTEGER NOT NULL,
            karma_min INTEGER DEFAULT -1000,
            karma_max INTEGER DEFAULT 1000,
            is_racial_class BOOLEAN DEFAULT FALSE,
            is_inflicted_class BOOLEAN DEFAULT FALSE,
            stat_bonuses JSON,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Character class schema creation completed successfully.")
        
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
    create_character_class_schema()

if __name__ == "__main__":
    main()