# ./DatabaseSetup/createRacesSchema.py

import sqlite3
import json
import os
from typing import List, Dict
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class RacesSystemInitializer:
    def __init__(self):
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
        """Create race-related tables"""
        try:
            print("Creating races system tables...")
            
            # Create races table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS races (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL UNIQUE,
                category TEXT NOT NULL CHECK(category IN ('Humanoid', 'Demi-Human', 'Heteromorphic')),
                description TEXT,
                max_racial_level INTEGER NOT NULL DEFAULT 1,
                stat_multiplier REAL NOT NULL DEFAULT 1.0,
                social_penalty INTEGER DEFAULT 0,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """)

            # Create racial_traits table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS racial_traits (
                id INTEGER PRIMARY KEY,
                race_id INTEGER NOT NULL,
                name TEXT NOT NULL,
                description TEXT,
                level_required INTEGER DEFAULT 1,
                effect_type TEXT NOT NULL,
                effect_value TEXT NOT NULL,
                is_passive BOOLEAN DEFAULT TRUE,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (race_id) REFERENCES races(id)
            )
            """)

            # Create race_evolution_paths table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS race_evolution_paths (
                id INTEGER PRIMARY KEY,
                base_race_id INTEGER NOT NULL,
                evolved_race_id INTEGER NOT NULL,
                required_level INTEGER NOT NULL,
                required_items TEXT,
                required_achievements TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (base_race_id) REFERENCES races(id),
                FOREIGN KEY (evolved_race_id) REFERENCES races(id)
            )
            """)

            # Create character_racial_traits table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS character_racial_traits (
                id INTEGER PRIMARY KEY,
                character_id INTEGER NOT NULL,
                racial_trait_id INTEGER NOT NULL,
                is_active BOOLEAN DEFAULT TRUE,
                acquired_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (character_id) REFERENCES characters(id),
                FOREIGN KEY (racial_trait_id) REFERENCES racial_traits(id)
            )
            """)

            self.conn.commit()
            print("Races system tables created successfully")
            return True
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error creating races tables: {e}")
            return False

    def initialize_base_races(self):
        """Initialize basic races"""
        base_races = [
            # Humanoid Races
            {
                'name': 'Human',
                'category': 'Humanoid',
                'description': 'Versatile and adaptable race with balanced attributes',
                'max_racial_level': 1,
                'stat_multiplier': 1.0,
                'social_penalty': 0
            },
            {
                'name': 'Elf',
                'category': 'Humanoid',
                'description': 'Long-lived race with affinity for magic',
                'max_racial_level': 1,
                'stat_multiplier': 1.0,
                'social_penalty': 0
            },
            # Demi-Human Races
            {
                'name': 'Dragonborn',
                'category': 'Demi-Human',
                'description': 'Warriors with dragon blood',
                'max_racial_level': 5,
                'stat_multiplier': 2.0,
                'social_penalty': 2
            },
            {
                'name': 'Werewolf',
                'category': 'Demi-Human',
                'description': 'Shapeshifters with enhanced physical abilities',
                'max_racial_level': 5,
                'stat_multiplier': 2.0,
                'social_penalty': 3
            },
            # Heteromorphic Races
            {
                'name': 'Dragon',
                'category': 'Heteromorphic',
                'description': 'Ancient beings of immense power',
                'max_racial_level': 10,
                'stat_multiplier': 3.0,
                'social_penalty': 5
            },
            {
                'name': 'Lich',
                'category': 'Heteromorphic',
                'description': 'Undead spellcasters of great power',
                'max_racial_level': 10,
                'stat_multiplier': 3.0,
                'social_penalty': 6
            }
        ]

        try:
            for race in base_races:
                self.cursor.execute("""
                INSERT OR IGNORE INTO races (
                    name, category, description, max_racial_level,
                    stat_multiplier, social_penalty
                ) VALUES (?, ?, ?, ?, ?, ?)
                """, (
                    race['name'], race['category'], race['description'],
                    race['max_racial_level'], race['stat_multiplier'],
                    race['social_penalty']
                ))
            
            self.conn.commit()
            print("Base races initialized successfully")
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error initializing base races: {e}")
            raise

    def initialize_racial_traits(self):
        """Initialize racial traits for base races"""
        # First get race IDs
        race_ids = {}
        self.cursor.execute("SELECT id, name FROM races")
        for row in self.cursor.fetchall():
            race_ids[row[1]] = row[0]

        racial_traits = [
            # Human Traits
            {
                'race_name': 'Human',
                'traits': [
                    {
                        'name': 'Adaptability',
                        'description': 'Increased experience gain',
                        'level_required': 1,
                        'effect_type': 'exp_bonus',
                        'effect_value': '10'
                    }
                ]
            },
            # Dragon Traits
            {
                'race_name': 'Dragon',
                'traits': [
                    {
                        'name': 'Dragon Scales',
                        'description': 'Natural armor',
                        'level_required': 1,
                        'effect_type': 'defense_bonus',
                        'effect_value': '50'
                    },
                    {
                        'name': 'Dragon Breath',
                        'description': 'Powerful breath attack',
                        'level_required': 5,
                        'effect_type': 'special_attack',
                        'effect_value': 'damage:100;type:fire',
                        'is_passive': False
                    }
                ]
            }
        ]

        try:
            for race_traits in racial_traits:
                race_id = race_ids.get(race_traits['race_name'])
                if race_id:
                    for trait in race_traits['traits']:
                        self.cursor.execute("""
                        INSERT INTO racial_traits (
                            race_id, name, description, level_required,
                            effect_type, effect_value, is_passive
                        ) VALUES (?, ?, ?, ?, ?, ?, ?)
                        """, (
                            race_id, trait['name'], trait['description'],
                            trait['level_required'], trait['effect_type'],
                            trait['effect_value'], trait.get('is_passive', True)
                        ))
            
            self.conn.commit()
            print("Racial traits initialized successfully")
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error initializing racial traits: {e}")
            raise

    def setup_races_system(self):
        """Main method to set up the races system"""
        try:
            self.connect_db()
            
            print("Starting races system initialization...")
            
            if not self.create_tables():
                raise Exception("Failed to create required tables")
            
            self.initialize_base_races()
            self.initialize_racial_traits()
            
            print("Races system initialization completed successfully")
            
        except Exception as e:
            print(f"Error during initialization: {e}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    try:
        initializer = RacesSystemInitializer()
        initializer.setup_races_system()
    except Exception as e:
        print(f"Failed to initialize races system: {e}")
        exit(1)