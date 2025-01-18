# ./DatabaseSetup/createPrerequisiteSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_prerequisite_schema():
    """Create the class prerequisites table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating prerequisite schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create prerequisites table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_prerequisites (
            id INTEGER PRIMARY KEY,
            class_id INTEGER NOT NULL,
            required_class_id INTEGER NOT NULL,
            required_level INTEGER NOT NULL,
            required_achievement_id INTEGER,
            required_quest_id INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_id) REFERENCES classes(id),
            FOREIGN KEY (required_class_id) REFERENCES classes(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Prerequisite schema creation completed successfully.")
        
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
    create_prerequisite_schema()

if __name__ == "__main__":
    main()