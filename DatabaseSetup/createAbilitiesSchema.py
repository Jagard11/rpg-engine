# ./DatabaseSetup/createAbilitiesSchema.py

import sqlite3
import json
import os
from typing import List, Dict
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class AbilitiesSystemInitializer:
    def __init__(self):
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        self.db_path = os.path.join(parent_dir, 'rpg_data.db')
        print(f"Using database path: {self.db_path}")
        self.conn = None
        self.cursor = None

    def connect_db(self):
        """Establish database connection"""
        try:
            self.conn = sqlite3.connect(self.db_path)
            self.cursor = self.conn.cursor()
            print(f"Connected to database: {self.db_path}")
        except sqlite3.Error as e:
            print(f"Database connection error: {e}")
            raise

    def close_db(self):
        """Close database connection"""
        if self.conn:
            self.conn.close()
            print("Database connection closed")

    def create_tables(self):
        """Create ability-related tables"""
        try:
            print("Creating ability system tables...")
            
            # Create abilities table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS abilities (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT,
                type TEXT NOT NULL CHECK(type IN ('Active', 'Passive', 'Special')),
                cooldown INTEGER,
                mana_cost INTEGER,
                damage INTEGER,
                healing INTEGER,
                duration INTEGER,
                range INTEGER,
                area_effect TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """)

            # Create character_abilities junction table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS character_abilities (
                id INTEGER PRIMARY KEY,
                character_id INTEGER NOT NULL,
                ability_id INTEGER NOT NULL,
                acquired_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                current_cooldown INTEGER DEFAULT 0,
                times_used INTEGER DEFAULT 0,
                FOREIGN KEY (character_id) REFERENCES characters(id),
                FOREIGN KEY (ability_id) REFERENCES abilities(id)
            )
            """)

            # Create ability_requirements table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS ability_requirements (
                id INTEGER PRIMARY KEY,
                ability_id INTEGER NOT NULL,
                required_level INTEGER NOT NULL DEFAULT 1,
                required_class_id INTEGER,
                required_race_id INTEGER,
                required_stat_type TEXT,
                required_stat_value INTEGER,
                FOREIGN KEY (ability_id) REFERENCES abilities(id),
                FOREIGN KEY (required_class_id) REFERENCES character_classes(id),
                FOREIGN KEY (required_race_id) REFERENCES races(id)
            )
            """)

            # Create ability_effects table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS ability_effects (
                id INTEGER PRIMARY KEY,
                ability_id INTEGER NOT NULL,
                effect_type TEXT NOT NULL,
                effect_value TEXT NOT NULL,
                duration INTEGER,
                save_type TEXT,
                save_dc INTEGER,
                FOREIGN KEY (ability_id) REFERENCES abilities(id)
            )
            """)

            self.conn.commit()
            print("Ability system tables created successfully")
            return True
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error creating ability tables: {e}")
            return False

    def initialize_base_abilities(self):
        """Initialize basic abilities"""
        base_abilities = [
            {
                'name': 'Quick Strike',
                'description': 'A swift attack that deals moderate damage',
                'type': 'Active',
                'cooldown': 3,
                'mana_cost': 0,
                'damage': 50,
                'healing': 0,
                'duration': 1,
                'range': 1,
                'area_effect': 'Single'
            },
            {
                'name': 'Healing Touch',
                'description': 'Restore health to a target',
                'type': 'Active',
                'cooldown': 5,
                'mana_cost': 30,
                'damage': 0,
                'healing': 100,
                'duration': 1,
                'range': 1,
                'area_effect': 'Single'
            },
            {
                'name': 'Battle Stance',
                'description': 'Increase physical damage dealt',
                'type': 'Passive',
                'cooldown': 0,
                'mana_cost': 0,
                'damage': 0,
                'healing': 0,
                'duration': 0,
                'range': 0,
                'area_effect': 'Self'
            }
        ]

        try:
            for ability in base_abilities:
                self.cursor.execute("""
                INSERT INTO abilities (
                    name, description, type, cooldown, mana_cost,
                    damage, healing, duration, range, area_effect
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    ability['name'], ability['description'], ability['type'],
                    ability['cooldown'], ability['mana_cost'], ability['damage'],
                    ability['healing'], ability['duration'], ability['range'],
                    ability['area_effect']
                ))
            
            self.conn.commit()
            print("Base abilities initialized successfully")
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error initializing base abilities: {e}")
            raise

    def setup_abilities_system(self):
        """Main method to set up the abilities system"""
        try:
            self.connect_db()
            
            print("Starting abilities system initialization...")
            
            if not self.create_tables():
                raise Exception("Failed to create required tables")
            
            self.initialize_base_abilities()
            
            print("Abilities system initialization completed successfully")
            
        except Exception as e:
            print(f"Error during initialization: {e}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    try:
        initializer = AbilitiesSystemInitializer()
        initializer.setup_abilities_system()
    except Exception as e:
        print(f"Failed to initialize abilities system: {e}")
        exit(1)