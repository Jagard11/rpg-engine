# ./DatabaseSetup/createClassSubcategorySchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_class_subcategory_schema():
    """Create the class subcategory table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating Class subcategory schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create prerequisites table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_subcategories (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)

        # insert default class subcategories
        default_subcategories = [
            # Race Subcategories
            (1, 'Magekin'),
            (2, 'Bioengineered'),
            (3, 'Construct'),

            # Job Subcategories
            (4, 'Academia'),
            (5, 'Trades'),
            (6, 'Fixed Wing Aircraft'),
            (7, 'Crew'),
        ]

        cursor.executemany("""
        INSERT OR IGNORE INTO class_subcategories (
            id, name            
        ) VALUES (?, ?)
        """, default_subcategories)
        
        # Commit changes
        conn.commit()
        logger.info("Class subcategory schema creation completed successfully.")
        
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
    create_class_subcategory_schema()

if __name__ == "__main__":
    main()