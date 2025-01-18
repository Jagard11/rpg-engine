# ./DatabaseSetup/createCharacterRaceProgressionSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_race_progression_schema():
    """Create the character race progression table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating character race progression schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create character race progression table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_race_progression (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            race_id INTEGER NOT NULL,
            current_level INTEGER NOT NULL DEFAULT 1,
            current_exp INTEGER NOT NULL DEFAULT 0,
            is_active BOOLEAN DEFAULT TRUE,
            unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            evolution_available BOOLEAN DEFAULT FALSE,
            next_evolution_id INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (race_id) REFERENCES races(id),
            FOREIGN KEY (next_evolution_id) REFERENCES races(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Character race progression schema creation completed successfully.")
        
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
    create_character_race_progression_schema()

if __name__ == "__main__":
    main()