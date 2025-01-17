# ./DatabaseSetup/createCharacterEquipmentSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_equipment_schema():
    """Create the character equipment table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating character equipment schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create character equipment table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_equipment (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            equipment_slot_id INTEGER NOT NULL,
            inventory_item_id INTEGER NOT NULL,
            equipped_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            slot_position INTEGER DEFAULT 0,
            is_transmog BOOLEAN DEFAULT FALSE,
            transmog_item_id INTEGER,
            active_set_bonus_ids JSON,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (equipment_slot_id) REFERENCES equipment_slots(id),
            FOREIGN KEY (inventory_item_id) REFERENCES character_inventory(id),
            FOREIGN KEY (transmog_item_id) REFERENCES items(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Character equipment schema creation completed successfully.")
        
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
    create_character_equipment_schema()

if __name__ == "__main__":
    main()