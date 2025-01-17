# ./DatabaseSetup/createRaceEvolutionSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_race_evolution_schema():
    """Create the race evolution paths table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating race evolution schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create race evolution paths table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS race_evolution_paths (
            id INTEGER PRIMARY KEY,
            base_race_id INTEGER NOT NULL,
            evolved_race_id INTEGER NOT NULL,
            required_level INTEGER NOT NULL,
            required_items TEXT,
            required_achievements TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (base_race_id) REFERENCES races(id),
            FOREIGN KEY (evolved_race_id) REFERENCES races(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Race evolution schema creation completed successfully.")
        
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
    create_race_evolution_schema()

if __name__ == "__main__":
    main()