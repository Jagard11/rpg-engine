# ./DatabaseSetup/createSpellSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_spell_schema():
    """Create the spells table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating spell schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create spells table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS spells (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            school_id INTEGER NOT NULL,
            tier_id INTEGER NOT NULL,
            base_mana_cost INTEGER NOT NULL,
            cast_time TEXT NOT NULL,
            range TEXT NOT NULL,
            area_of_effect TEXT,
            duration TEXT NOT NULL,
            damage_type_id INTEGER,
            base_damage TEXT,
            healing_amount TEXT,
            is_concentration BOOLEAN DEFAULT FALSE,
            is_ritual BOOLEAN DEFAULT FALSE,
            components TEXT NOT NULL,
            material_components TEXT,
            can_be_interrupted BOOLEAN DEFAULT TRUE,
            interruption_dc INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (school_id) REFERENCES magic_schools(id),
            FOREIGN KEY (tier_id) REFERENCES spell_tiers(id),
            FOREIGN KEY (damage_type_id) REFERENCES damage_types(id)
        )
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Spell schema creation completed successfully.")
        
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
    create_spell_schema()

if __name__ == "__main__":
    main()