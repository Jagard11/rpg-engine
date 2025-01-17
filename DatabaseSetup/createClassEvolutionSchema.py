# ./DatabaseSetup/createClassEvolutionSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_class_evolution_schema():
    """Create the class evolution paths table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating class evolution schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create class evolution paths table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_evolution_paths (
            id INTEGER PRIMARY KEY,
            base_class_id INTEGER NOT NULL,
            evolved_class_id INTEGER NOT NULL,
            required_level INTEGER NOT NULL,
            required_items TEXT,
            required_achievements TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (base_class_id) REFERENCES character_classes(id),
            FOREIGN KEY (evolved_class_id) REFERENCES character_classes(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Class evolution schema creation completed successfully.")
        
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
    create_class_evolution_schema()

if __name__ == "__main__":
    main()