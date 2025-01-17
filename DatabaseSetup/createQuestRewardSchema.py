# ./DatabaseSetup/createQuestRewardSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_quest_reward_schema():
    """Create the quest rewards table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating quest reward schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create quest rewards table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS quest_rewards (
            id INTEGER PRIMARY KEY,
            quest_id INTEGER NOT NULL,
            reward_type TEXT NOT NULL CHECK(reward_type IN ('Experience', 'Gold', 'Item', 'Ability', 'Reputation', 'Title', 'Custom')),
            reward_id INTEGER,
            quantity INTEGER DEFAULT 1,
            chance REAL DEFAULT 100.0,
            is_choice_reward BOOLEAN DEFAULT FALSE,
            choice_group INTEGER,
            custom_data JSON,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (quest_id) REFERENCES quests(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Quest reward schema creation completed successfully.")
        
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
    create_quest_reward_schema()

if __name__ == "__main__":
    main()