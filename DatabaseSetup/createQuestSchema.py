# ./DatabaseSetup/createQuestSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_quest_schema():
    """Create the quests table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating quest schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create quests table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS quests (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            type TEXT NOT NULL CHECK(type IN ('Main', 'Side', 'Daily', 'Event', 'Hidden')),
            level_requirement INTEGER DEFAULT 1,
            quest_chain_id INTEGER,
            quest_chain_order INTEGER,
            karma_requirement INTEGER DEFAULT 0,
            is_repeatable BOOLEAN DEFAULT FALSE,
            cooldown_hours INTEGER,
            time_limit_minutes INTEGER,
            difficulty TEXT CHECK(difficulty IN ('Easy', 'Normal', 'Hard', 'Expert')),
            story_text TEXT,
            completion_text TEXT,
            is_active BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (quest_chain_id) REFERENCES quests(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Quest schema creation completed successfully.")
        
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
    create_quest_schema()

if __name__ == "__main__":
    main()