# ./SchemaManager/schemas/createSpellTypeSchema.py

import sqlite3
import os
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_spell_type_schema():
    try:
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS spell_types (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)

        # Default spell types
        default_types = [
            (1, 'melee'),
            (2, 'special'),
            (3, 'spell'),
            (4, 'aura'),
            (5, 'talent'),
            (6, 'skill')
        ]

        cursor.executemany("""
        INSERT OR IGNORE INTO spell_types (id, name) 
        VALUES (?, ?)
        """, default_types)
        
        # Add spell_type_id to spells table if it doesn't exist
        cursor.execute("PRAGMA table_info(spells)")
        columns = [col[1] for col in cursor.fetchall()]
        
        if 'spell_type_id' not in columns:
            cursor.execute("""
            ALTER TABLE spells 
            ADD COLUMN spell_type_id INTEGER REFERENCES spell_types(id)
            """)

        conn.commit()
        logger.info("Spell type schema creation completed successfully.")
        
    except Exception as e:
        logger.error(f"An error occurred: {str(e)}")
        raise
    finally:
        if conn:
            conn.close()

def main():
    create_spell_type_schema()

if __name__ == "__main__":
    main()