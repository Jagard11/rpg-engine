# ./DatabaseSetup/createAbilityRequirementSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_ability_requirement_schema():
    """Create the ability requirements table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating ability requirement schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create ability requirements table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS ability_requirements (
            id INTEGER PRIMARY KEY,
            ability_id INTEGER NOT NULL,
            required_level INTEGER NOT NULL DEFAULT 1,
            required_class_id INTEGER,
            required_race_id INTEGER,
            required_stat_type TEXT,
            required_stat_value INTEGER,
            FOREIGN KEY (ability_id) REFERENCES abilities(id),
            FOREIGN KEY (required_class_id) REFERENCES character_classes(id),
            FOREIGN KEY (required_race_id) REFERENCES races(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Ability requirement schema creation completed successfully.")
        
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
    create_ability_requirement_schema()

if __name__ == "__main__":
    main()