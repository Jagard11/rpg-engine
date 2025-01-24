# ./SchemaManager/schemas/createSpellTiersSchema.py

import sqlite3
import os
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_spell_tiers_schema():
    try:
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS spell_tiers (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            min_level INTEGER NOT NULL,
            max_slots INTEGER NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)

        # Default spell tiers
        default_tiers = [
            (0, 'Cantrip', 'Basic spells that can be cast at will', 0, 17),
            (1, 'Tier 1', 'First tier spells', 7, 15),
            (2, 'Tier 2', 'Second tier spells', 14, 14),
            (3, 'Tier 3', 'Third tier spells', 21, 13),
            (4, 'Tier 4', 'Fourth tier spells', 28, 12),
            (5, 'Tier 5', 'Fifth tier spells', 35, 11),
            (6, 'Tier 6', 'Sixth tier spells', 42, 10),
            (7, 'Tier 7', 'Seventh tier spells', 49, 9),
            (8, 'Tier 8', 'Eighth tier spells', 56, 8),
            (9, 'Tier 9', 'Ninth tier spells', 63, 7),
            (10, 'Tier 10', 'Tenth tier spells', 70, 6),
            (11, 'Super Tier', 'Most powerful spells', 77, 5)
        ]

        cursor.executemany("""
        INSERT OR IGNORE INTO spell_tiers (id, name, description, min_level, max_slots) 
        VALUES (?, ?, ?, ?, ?)
        """, default_tiers)
        
        conn.commit()
        logger.info("Spell tiers schema creation completed successfully.")
        
    except Exception as e:
        logger.error(f"An error occurred: {str(e)}")
        raise
    finally:
        if conn:
            conn.close()

def main():
    create_spell_tiers_schema()

if __name__ == "__main__":
    main()