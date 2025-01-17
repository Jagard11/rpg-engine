# ./DatabaseSetup/createTalentEffectSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_talent_effect_schema():
    """Create the talent effects table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating talent effect schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create talent effects table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS talent_effects (
            id INTEGER PRIMARY KEY,
            talent_id INTEGER NOT NULL,
            effect_type TEXT NOT NULL,
            effect_value TEXT NOT NULL,
            duration INTEGER,
            is_hidden BOOLEAN DEFAULT FALSE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (talent_id) REFERENCES talents(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Talent effect schema creation completed successfully.")
        
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
    create_talent_effect_schema()

if __name__ == "__main__":
    main()