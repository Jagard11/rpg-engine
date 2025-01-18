# ./DatabaseSetup/createCharacterStatsSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_stats_schema():  # This name matches what initializeSchema.py expects
    """Create the character stats table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating character stats schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create character stats table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_stats (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            base_hp INTEGER NOT NULL DEFAULT 0,
            current_hp INTEGER NOT NULL DEFAULT 0,
            base_mp INTEGER NOT NULL DEFAULT 0,
            current_mp INTEGER NOT NULL DEFAULT 0,
            base_physical_attack INTEGER NOT NULL DEFAULT 0,
            current_physical_attack INTEGER NOT NULL DEFAULT 0,
            base_physical_defense INTEGER NOT NULL DEFAULT 0,
            current_physical_defense INTEGER NOT NULL DEFAULT 0,
            base_agility INTEGER NOT NULL DEFAULT 0,
            current_agility INTEGER NOT NULL DEFAULT 0,
            base_magical_attack INTEGER NOT NULL DEFAULT 0,
            current_magical_attack INTEGER NOT NULL DEFAULT 0,
            base_magical_defense INTEGER NOT NULL DEFAULT 0,
            current_magical_defense INTEGER NOT NULL DEFAULT 0,
            base_resistance INTEGER NOT NULL DEFAULT 0,
            current_resistance INTEGER NOT NULL DEFAULT 0,
            base_special INTEGER NOT NULL DEFAULT 0,
            current_special INTEGER NOT NULL DEFAULT 0,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Character stats schema creation completed successfully.")
        
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
    create_character_stats_schema()

if __name__ == "__main__":
    main()