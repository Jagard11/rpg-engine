# ./database/createSpellSchema.py

import sqlite3
from pathlib import Path
import logging
from datetime import datetime

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_spell_schema():
    """Create the spell system database schema"""
    try:
        # Ensure database directory exists
        db_dir = Path('database')
        db_dir.mkdir(exist_ok=True)
        
        # Connect to database
        conn = sqlite3.connect('database/rpg_data.db')
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        logger.info("Creating spell system schema...")
        
        # Create magic schools table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS magic_schools (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            description TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        
        # Create spell tiers table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS spell_tiers (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            level INTEGER NOT NULL,
            mana_cost_multiplier REAL DEFAULT 1.0,
            slot_count INTEGER NOT NULL,
            min_class_level INTEGER NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        
        # Create damage types table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS damage_types (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            description TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        
        # Create spells table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS spells (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
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
        
        # Create spell effects table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS spell_effects (
            id INTEGER PRIMARY KEY,
            spell_id INTEGER NOT NULL,
            effect_type TEXT NOT NULL,
            effect_value TEXT NOT NULL,
            duration TEXT,
            save_type TEXT,
            save_dc INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (spell_id) REFERENCES spells(id)
        )
        """)
        
        # Create character_spells junction table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_spells (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            spell_id INTEGER NOT NULL,
            is_prepared BOOLEAN DEFAULT FALSE,
            times_cast INTEGER DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (spell_id) REFERENCES spells(id)
        )
        """)
        
        # Create class_spells junction table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_spells (
            id INTEGER PRIMARY KEY,
            class_id INTEGER NOT NULL,
            spell_id INTEGER NOT NULL,
            min_level INTEGER NOT NULL DEFAULT 1,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_id) REFERENCES character_classes(id),
            FOREIGN KEY (spell_id) REFERENCES spells(id)
        )
        """)
        
        logger.info("Tables created successfully. Inserting initial data...")
        
        # Insert magic schools
        magic_schools = [
            ('Arcane', 'Traditional magic that manipulates fundamental forces'),
            ('Divine', 'Magic powered by deities and faith'),
            ('Spiritual', 'Magic dealing with spirits, souls, and nature'),
            ('Alternative', 'Unique and rare forms of magic')
        ]
        
        cursor.executemany(
            "INSERT OR IGNORE INTO magic_schools (name, description) VALUES (?, ?)",
            magic_schools
        )
        
        # Insert spell tiers
        spell_tiers = [
            ('Cantrip', 0, 17, 1),
            ('Tier 1', 1, 15, 1),
            ('Tier 2', 2, 14, 7),
            ('Tier 3', 3, 13, 14),
            ('Tier 4', 4, 12, 21),
            ('Tier 5', 5, 11, 28),
            ('Tier 6', 6, 10, 35),
            ('Tier 7', 7, 9, 42),
            ('Tier 8', 8, 8, 49),
            ('Tier 9', 9, 7, 56),
            ('Tier 10', 10, 6, 63),
            ('Super Tier', 11, 5, 70)
        ]
        
        cursor.executemany(
            "INSERT OR IGNORE INTO spell_tiers (name, level, slot_count, min_class_level) VALUES (?, ?, ?, ?)",
            spell_tiers
        )
        
        # Insert damage types
        damage_types = [
            ('Physical', 'Raw physical damage'),
            ('Fire', 'Fire and heat damage'),
            ('Ice', 'Cold and frost damage'),
            ('Lightning', 'Electrical damage'),
            ('Holy', 'Divine and holy damage'),
            ('Shadow', 'Dark and necrotic damage'),
            ('Force', 'Pure magical force damage'),
            ('Poison', 'Toxic and venomous damage')
        ]
        
        cursor.executemany(
            "INSERT OR IGNORE INTO damage_types (name, description) VALUES (?, ?)",
            damage_types
        )
        
        # Commit changes and close connection
        conn.commit()
        logger.info("Schema creation and initial data insertion completed successfully.")
        
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

if __name__ == "__main__":
    try:
        create_spell_schema()
    except Exception as e:
        logger.error(f"Failed to create spell schema: {str(e)}")
        exit(1)