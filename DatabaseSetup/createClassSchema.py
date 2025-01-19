# ./DatabaseSetup/createClassSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_class_schema():
    """Create the classes table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating class schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create classes table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS classes (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            class_type INTEGER NOT NULL,
            is_racial BOOLEAN DEFAULT FALSE,
            category TEXT,
            subcategory TEXT,
            base_hp INTEGER NOT NULL DEFAULT 0,
            base_mp INTEGER NOT NULL DEFAULT 0,
            base_physical_attack INTEGER NOT NULL DEFAULT 0,
            base_physical_defense INTEGER NOT NULL DEFAULT 0,
            base_agility INTEGER NOT NULL DEFAULT 0,
            base_magical_attack INTEGER NOT NULL DEFAULT 0,
            base_magical_defense INTEGER NOT NULL DEFAULT 0,
            base_resistance INTEGER NOT NULL DEFAULT 0,
            base_special INTEGER NOT NULL DEFAULT 0,
            karma_requirement_min INTEGER DEFAULT -1000,
            karma_requirement_max INTEGER DEFAULT 1000,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_type) REFERENCES class_categories(id)
        )
        """)

        # Insert default classes
        default_classes = [
            # Racial Classes
            (1, 'Human', 'Basic human attributes and capabilities', 1, True, 'Humanoid', 'Human', 10, 5, 5, 5, 5, 5, 5, 5, 5, -1000, 1000),
            
            # Job Classes
            (2, 'Student', 'Basic academic knowledge and study skills', 1, False, 'Academic', None, 5, 10, 2, 2, 3, 8, 5, 5, 5, -1000, 1000),
            (3, 'Welder', 'Rudimentary welding capabilities', 1, False, 'Craft', 'Metal', 8, 3, 7, 5, 4, 2, 3, 4, 5, -1000, 1000),
            (4, 'Flight Simulator Enthusiast', 'Basic understanding of flight principles', 1, False, 'Pilot', 'Simulation', 5, 8, 3, 3, 6, 5, 4, 4, 7, -1000, 1000),
            (5, 'Ship Operations Trainee', 'Beginning to learn Terevolost systems', 1, False, 'Pilot', 'Ship', 7, 7, 4, 4, 5, 6, 5, 5, 6, -1000, 1000)
        ]

        cursor.executemany("""
        INSERT OR IGNORE INTO classes (
            id, name, description, class_type, is_racial, category, subcategory,
            base_hp, base_mp, base_physical_attack, base_physical_defense,
            base_agility, base_magical_attack, base_magical_defense,
            base_resistance, base_special,
            karma_requirement_min, karma_requirement_max
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, default_classes)
        
        # Commit changes
        conn.commit()
        logger.info("Class schema creation completed successfully.")
        
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
    create_class_schema()

if __name__ == "__main__":
    main()