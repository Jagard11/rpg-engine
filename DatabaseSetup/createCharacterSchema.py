# ./database/createCharacterSchema.py

import sqlite3
from pathlib import Path
import logging
from datetime import datetime

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_character_schema():
    """Create the character system database schema"""
    try:
        # Ensure database directory exists
        db_dir = Path('database')
        db_dir.mkdir(exist_ok=True)
        
        # Connect to database
        conn = sqlite3.connect('database/rpg_data.db')
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        logger.info("Creating character system schema...")
        
        # Create races table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS races (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            category TEXT NOT NULL CHECK(category IN ('Humanoid', 'Demi-Human', 'Heteromorphic')),
            description TEXT,
            max_racial_level INTEGER NOT NULL,
            stat_multiplier REAL NOT NULL,
            social_penalty INTEGER DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        
        # Create character_classes table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_classes (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            type TEXT NOT NULL CHECK(type IN ('Base', 'High', 'Rare')),
            description TEXT,
            max_level INTEGER NOT NULL,
            prerequisite_class_id INTEGER,
            min_karma INTEGER DEFAULT -1000,
            max_karma INTEGER DEFAULT 1000,
            quest_requirement_id INTEGER,
            is_inflicted BOOLEAN DEFAULT FALSE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (prerequisite_class_id) REFERENCES character_classes(id)
        )
        """)
        
        # Create characters table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS characters (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            race_id INTEGER NOT NULL,
            racial_level INTEGER DEFAULT 1,
            current_class_id INTEGER NOT NULL,
            karma INTEGER DEFAULT 0,
            level INTEGER DEFAULT 1,
            experience INTEGER DEFAULT 0,
            current_health INTEGER NOT NULL,
            max_health INTEGER NOT NULL,
            current_mana INTEGER NOT NULL,
            max_mana INTEGER NOT NULL,
            strength INTEGER NOT NULL,
            dexterity INTEGER NOT NULL,
            intelligence INTEGER NOT NULL,
            constitution INTEGER NOT NULL,
            physical_attack INTEGER NOT NULL,
            physical_defense INTEGER NOT NULL,
            agility INTEGER NOT NULL,
            magical_attack INTEGER NOT NULL,
            magical_defense INTEGER NOT NULL,
            resistance INTEGER NOT NULL,
            special_ability INTEGER NOT NULL,
            description TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (race_id) REFERENCES races(id),
            FOREIGN KEY (current_class_id) REFERENCES character_classes(id)
        )
        """)
        
        # Create character_classes_progression table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_classes_progression (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            class_id INTEGER NOT NULL,
            level INTEGER DEFAULT 1,
            is_active BOOLEAN DEFAULT FALSE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (class_id) REFERENCES character_classes(id)
        )
        """)
        
        # Create talents table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS talents (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            description TEXT,
            is_combat_applicable BOOLEAN DEFAULT FALSE,
            required_level INTEGER DEFAULT 1,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        """)
        
        # Create character_talents table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_talents (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            talent_id INTEGER NOT NULL,
            is_discovered BOOLEAN DEFAULT FALSE,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (talent_id) REFERENCES talents(id)
        )
        """)

        # Create skills table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS skills (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            type TEXT NOT NULL CHECK(type IN ('Active', 'Passive', 'Monster', 'Martial')),
            description TEXT,
            daily_uses INTEGER,
            recharge_time INTEGER,
            mana_cost INTEGER DEFAULT 0,
            class_requirement_id INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_requirement_id) REFERENCES character_classes(id)
        )
        """)
        
        # Create character_skills table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS character_skills (
            id INTEGER PRIMARY KEY,
            character_id INTEGER NOT NULL,
            skill_id INTEGER NOT NULL,
            current_uses INTEGER,
            last_used TIMESTAMP,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters(id),
            FOREIGN KEY (skill_id) REFERENCES skills(id)
        )
        """)

        logger.info("Tables created successfully. Inserting initial data...")
        
        # Insert base race categories
        races = [
            ('Human', 'Humanoid', 'Versatile and adaptable race', 1, 1.0, 0),
            ('Elf', 'Humanoid', 'Long-lived and magically attuned race', 1, 1.0, 0),
            ('Dwarf', 'Humanoid', 'Hardy mountain-dwelling race', 1, 1.0, 0),
            ('Orc', 'Demi-Human', 'Powerful warrior race', 5, 2.0, 2),
            ('Lizardman', 'Demi-Human', 'Scaled warriors with natural armor', 5, 2.0, 2),
            ('Vampire', 'Heteromorphic', 'Undead beings with great power', 10, 3.0, 5),
            ('Dragon', 'Heteromorphic', 'Ancient beings of immense power', 10, 3.0, 5)
        ]
        
        cursor.executemany(
            "INSERT OR IGNORE INTO races (name, category, description, max_racial_level, stat_multiplier, social_penalty) VALUES (?, ?, ?, ?, ?, ?)",
            races
        )
        
        # Insert base character classes
        classes = [
            ('Warrior', 'Base', 'Master of weapons and combat', 15, None, -1000, 1000, None, False),
            ('Mage', 'Base', 'Wielder of arcane magic', 15, None, -1000, 1000, None, False),
            ('Rogue', 'Base', 'Expert in stealth and precision', 15, None, -1000, 1000, None, False),
            ('Paladin', 'High', 'Holy warrior blessed by the divine', 10, 1, 200, 1000, None, False),
            ('Necromancer', 'High', 'Master of death and undeath', 10, 2, -1000, -200, None, False),
            ('Dragon Knight', 'Rare', 'Warrior blessed with dragon power', 5, 4, 0, 1000, 1, False),
            ('Slave', 'Base', 'Bound servant class', 10, None, -1000, 0, None, True)
        ]
        
        cursor.executemany(
            "INSERT OR IGNORE INTO character_classes (name, type, description, max_level, prerequisite_class_id, min_karma, max_karma, quest_requirement_id, is_inflicted) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            classes
        )
        
        # Insert base talents
        talents = [
            ('Natural Leader', 'Innate ability to command and inspire others', True, 1),
            ('Quick Learner', 'Accelerated skill acquisition and mastery', False, 1),
            ('Magical Affinity', 'Natural attunement to magical forces', True, 1),
            ('Divine Blessing', 'Blessed by the gods from birth', True, 10)
        ]
        
        cursor.executemany(
            "INSERT OR IGNORE INTO talents (name, description, is_combat_applicable, required_level) VALUES (?, ?, ?, ?)",
            talents
        )
        
        # Insert base skills
        skills = [
            ('Power Strike', 'Active', 'Powerful melee attack with increased damage', 3, 3600, 0, 1),
            ('Stealth', 'Active', 'Move unseen in shadows', 5, 1800, 0, 3),
            ('Dragon Scales', 'Passive', 'Natural armor enhancement', None, None, 0, 6),
            ('Dual Wield', 'Martial', 'Fight effectively with two weapons', None, None, 0, 1)
        ]
        
        cursor.executemany(
            "INSERT OR IGNORE INTO skills (name, type, description, daily_uses, recharge_time, mana_cost, class_requirement_id) VALUES (?, ?, ?, ?, ?, ?, ?)",
            skills
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
        create_character_schema()
    except Exception as e:
        logger.error(f"Failed to create character schema: {str(e)}")
        exit(1)