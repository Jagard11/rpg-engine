# ./DatabaseSetup/createCharacterClassProgressionSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_class_progression_schema():
    """Create the character class progression table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating character class progression schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create character class progression table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_class_progression (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            class_id INTEGER NOT NULL,
            current_level INTEGER NOT NULL DEFAULT 0,
            current_experience INTEGER NOT NULL DEFAULT 0,
            unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id) ON DELETE CASCADE,
            FOREIGN KEY (class_id) REFERENCES classes(id),
            UNIQUE(character_id, class_id)
        )
        """)
        
        # Insert default progression for James Gerard
        default_progression = [
            # Racial Classes
            (1, 1, 1, 1, 0),  # Human [1]
            
            # Job Classes
            (2, 1, 2, 5, 0),  # Student [5]
            (3, 1, 3, 2, 0),  # Welder [2]
            (4, 1, 4, 2, 0),  # Flight Simulator Enthusiast [2]
            (5, 1, 5, 5, 0)   # Ship Operations Trainee [5]
        ]

        cursor.executemany("""
        INSERT OR IGNORE INTO character_class_progression (
            id, character_id, class_id, current_level, current_experience
        ) VALUES (?, ?, ?, ?, ?)
        """, default_progression)
        
        # Create index for faster lookups
        cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_char_class_prog_lookup 
        ON character_class_progression(character_id, class_id)
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Character class progression schema creation completed successfully.")
        
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
    create_character_class_progression_schema()

if __name__ == "__main__":
    main()