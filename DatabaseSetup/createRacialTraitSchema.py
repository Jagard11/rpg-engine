# ./DatabaseSetup/createRacialTraitSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_racial_trait_schema():
    """Create the racial traits table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating racial trait schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create racial traits table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS racial_traits (
            id INTEGER PRIMARY KEY,
            race_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            description TEXT,
            level_required INTEGER DEFAULT 1,
            effect_type TEXT NOT NULL,
            effect_value TEXT NOT NULL,
            is_passive BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (race_id) REFERENCES races(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Racial trait schema creation completed successfully.")
        
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
    create_racial_trait_schema()

if __name__ == "__main__":
    main()