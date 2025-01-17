# ./DatabaseSetup/createCharacterQuestSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_quest_schema():
    """Create the character quests table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating character quest schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create character quests table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_quests (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            quest_id INTEGER NOT NULL,
            status TEXT NOT NULL CHECK(status IN ('Available', 'Active', 'Completed', 'Failed', 'Abandoned')),
            progress JSON,
            started_at TIMESTAMP,
            completed_at TIMESTAMP,
            times_completed INTEGER DEFAULT 0,
            last_reset_at TIMESTAMP,
            selected_rewards JSON,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (quest_id) REFERENCES quests(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Character quest schema creation completed successfully.")
        
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
    create_character_quest_schema()

if __name__ == "__main__":
    main()