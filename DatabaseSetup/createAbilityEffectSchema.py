# ./DatabaseSetup/createAbilityEffectSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_ability_effect_schema():
    """Create the ability effects table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating ability effect schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create ability effects table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS ability_effects (
            id INTEGER PRIMARY KEY,
            ability_id INTEGER NOT NULL,
            effect_type TEXT NOT NULL,
            effect_value TEXT NOT NULL,
            duration INTEGER,
            save_type TEXT,
            save_dc INTEGER,
            FOREIGN KEY (ability_id) REFERENCES abilities(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Ability effect schema creation completed successfully.")
        
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
    create_ability_effect_schema()

if __name__ == "__main__":
    main()