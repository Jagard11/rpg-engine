# ./SchemaManager/schemas/createSpellSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_spell_schema():
    """Create the spells table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating spell schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create spells table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS spells (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            spell_tier INTEGER NOT NULL CHECK(spell_tier >= 0 AND spell_tier <= 10),
            is_super_tier BOOLEAN DEFAULT FALSE,
            mp_cost INTEGER NOT NULL DEFAULT 0,
            casting_time TEXT,
            range TEXT,
            area_of_effect TEXT,
            damage_base INTEGER,
            damage_scaling TEXT,
            healing_base INTEGER,
            healing_scaling TEXT,
            status_effects TEXT,
            duration TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Spell schema creation completed successfully.")
        
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
    create_spell_schema()

if __name__ == "__main__":
    main()