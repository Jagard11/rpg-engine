# ./DatabaseSetup/createCharacterSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_schema():
    """Create the characters table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating character schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create characters table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS characters (
            id INTEGER PRIMARY KEY,
            first_name TEXT NOT NULL,
            middle_name TEXT,
            last_name TEXT NOT NULL,
            bio TEXT,
            race_column TEXT,           
            total_level INTEGER NOT NULL DEFAULT 0,
            birth_place TEXT,
            age INTEGER,
            karma INTEGER DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            talent TEXT,
            is_active BOOLEAN DEFAULT TRUE
        )
        """)

        # Insert default character
        default_character = [
            (1, 'James', None, 'Gerard', 
             'A young student beginning his journey into space exploration', "humanoid",
             15, 'Earth', 19, 0, 'Adaptive Learning', True)
        ]

        cursor.executemany("""
        INSERT OR IGNORE INTO characters (
            id, first_name, middle_name, last_name, bio, race_column,
            total_level, birth_place, age, karma, talent, is_active
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, default_character)
        
        # Commit changes
        conn.commit()
        logger.info("Character schema creation completed successfully.")
        
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
    create_character_schema()

if __name__ == "__main__":
    main()