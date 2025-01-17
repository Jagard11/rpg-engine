# ./DatabaseSetup/createClassesSchema.py

import sqlite3
import json
import os
from typing import List, Dict
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ClassesSystemInitializer:
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
        """Create class-related tables"""
        try:
            print("Creating classes system tables...")
            
            # Create character_classes table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS character_classes (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL UNIQUE,
                type TEXT CHECK(type IN ('Base', 'High', 'Rare')) NOT NULL,
                description TEXT,
                max_level INTEGER NOT NULL,
                karma_min INTEGER DEFAULT -1000,
                karma_max INTEGER DEFAULT 1000,
                is_racial_class BOOLEAN DEFAULT FALSE,
                is_inflicted_class BOOLEAN DEFAULT FALSE,
                stat_bonuses JSON,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """)

            # Create class_levels table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS class_levels (
                id INTEGER PRIMARY KEY,
                class_id INTEGER NOT NULL,
                level INTEGER NOT NULL,
                exp_required INTEGER NOT NULL,
                hp_bonus INTEGER NOT NULL DEFAULT 0,
                mp_bonus INTEGER NOT NULL DEFAULT 0,
                stat_bonuses JSON,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (class_id) REFERENCES character_classes(id)
            )
            """)

            # Create class_prerequisites table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS class_prerequisites (
                id INTEGER PRIMARY KEY,
                class_id INTEGER NOT NULL,
                required_class_id INTEGER NOT NULL,
                required_level INTEGER NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (class_id) REFERENCES character_classes(id),
                FOREIGN KEY (required_class_id) REFERENCES character_classes(id)
            )
            """)

            # Create class_evolution_paths table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS class_evolution_paths (
                id INTEGER PRIMARY KEY,
                base_class_id INTEGER NOT NULL,
                evolved_class_id INTEGER NOT NULL,
                required_level INTEGER NOT NULL,
                required_items TEXT,
                required_achievements TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (base_class_id) REFERENCES character_classes(id),
                FOREIGN KEY (evolved_class_id) REFERENCES character_classes(id)
            )
            """)

            # Create character_class_progression table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS character_class_progression (
                id INTEGER PRIMARY KEY,
                character_id INTEGER NOT NULL,
                class_id INTEGER NOT NULL,
                current_level INTEGER NOT NULL DEFAULT 1,
                current_exp INTEGER NOT NULL DEFAULT 0,
                is_active BOOLEAN DEFAULT FALSE,
                unlocked_at TIMESTAMP,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (character_id) REFERENCES characters(id),
                FOREIGN KEY (class_id) REFERENCES character_classes(id)
            )
            """)

            # Create class_abilities table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS class_abilities (
                id INTEGER PRIMARY KEY,
                class_id INTEGER NOT NULL,
                ability_id INTEGER NOT NULL,
                required_level INTEGER NOT NULL DEFAULT 1,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (class_id) REFERENCES character_classes(id),
                FOREIGN KEY (ability_id) REFERENCES abilities(id)
            )
            """)

            self.conn.commit()
            print("Classes system tables created successfully")
            return True
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error creating classes tables: {e}")
            return False

    def initialize_base_classes(self):
        """Initialize basic classes"""
        base_classes = [
            # Base Classes (max level 15)
            {
                'name': 'Warrior',
                'type': 'Base',
                'description': 'Master of weapons and physical combat',
                'max_level': 15,
                'stat_bonuses': {
                    'strength': 3,
                    'constitution': 2,
                    'physical_attack': 2
                }
            },
            {
                'name': 'Mage',
                'type': 'Base',
                'description': 'Master of arcane magic',
                'max_level': 15,
                'stat_bonuses': {
                    'intelligence': 3,
                    'wisdom': 2,
                    'magical_attack': 2
                }
            },
            # High Classes (max level 10)
            {
                'name': 'Paladin',
                'type': 'High',
                'description': 'Holy warrior with divine powers',
                'max_level': 10,
                'karma_min': 200,
                'stat_bonuses': {
                    'strength': 2,
                    'constitution': 2,
                    'wisdom': 2,
                    'holy_power': 3
                }
            },
            {
                'name': 'Necromancer',
                'type': 'High',
                'description': 'Master of death magic',
                'max_level': 10,
                'karma_max': -200,
                'stat_bonuses': {
                    'intelligence': 3,
                    'wisdom': 2,
                    'death_magic': 3
                }
            },
            # Rare Classes (max level 5)
            {
                'name': 'Dragon Knight',
                'type': 'Rare',
                'description': 'Warrior blessed with dragon powers',
                'max_level': 5,
                'stat_bonuses': {
                    'strength': 4,
                    'constitution': 3,
                    'dragon_affinity': 3
                }
            }
        ]

        try:
            for class_data in base_classes:
                # Insert the class
                self.cursor.execute("""
                INSERT INTO character_classes (
                    name, type, description, max_level,
                    karma_min, karma_max, stat_bonuses
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """, (
                    class_data['name'],
                    class_data['type'],
                    class_data['description'],
                    class_data['max_level'],
                    class_data.get('karma_min', -1000),
                    class_data.get('karma_max', 1000),
                    json.dumps(class_data['stat_bonuses'])
                ))
                
                class_id = self.cursor.lastrowid
                
                # Initialize level progression for this class
                for level in range(1, class_data['max_level'] + 1):
                    exp_required = level * 1000  # Simple progression for example
                    hp_bonus = level * 10
                    mp_bonus = level * 5
                    level_stat_bonuses = {
                        stat: bonus * (level / 2)
                        for stat, bonus in class_data['stat_bonuses'].items()
                    }
                    
                    self.cursor.execute("""
                    INSERT INTO class_levels (
                        class_id, level, exp_required,
                        hp_bonus, mp_bonus, stat_bonuses
                    ) VALUES (?, ?, ?, ?, ?, ?)
                    """, (
                        class_id, level, exp_required,
                        hp_bonus, mp_bonus,
                        json.dumps(level_stat_bonuses)
                    ))
            
            self.conn.commit()
            print("Base classes initialized successfully")
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error initializing base classes: {e}")
            raise

    def initialize_class_prerequisites(self):
        """Initialize class prerequisites"""
        prerequisites = [
            # Prerequisites for High classes
            {
                'class_name': 'Paladin',
                'requirements': [
                    {'required_class': 'Warrior', 'level': 10}
                ]
            },
            {
                'class_name': 'Necromancer',
                'requirements': [
                    {'required_class': 'Mage', 'level': 10}
                ]
            },
            # Prerequisites for Rare classes
            {
                'class_name': 'Dragon Knight',
                'requirements': [
                    {'required_class': 'Warrior', 'level': 15},
                    {'required_class': 'Paladin', 'level': 5}
                ]
            }
        ]

        try:
            for prereq in prerequisites:
                # Get the class ID
                self.cursor.execute("SELECT id FROM character_classes WHERE name = ?", (prereq['class_name'],))
                class_id = self.cursor.fetchone()[0]
                
                # Add each requirement
                for req in prereq['requirements']:
                    self.cursor.execute("""
                    INSERT INTO class_prerequisites (
                        class_id, required_class_id, required_level
                    ) VALUES (
                        ?, 
                        (SELECT id FROM character_classes WHERE name = ?),
                        ?
                    )
                    """, (class_id, req['required_class'], req['level']))
            
            self.conn.commit()
            print("Class prerequisites initialized successfully")
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error initializing class prerequisites: {e}")
            raise

    def setup_classes_system(self):
        """Main method to set up the classes system"""
        try:
            self.connect_db()
            
            print("Starting classes system initialization...")
            
            if not self.create_tables():
                raise Exception("Failed to create required tables")
            
            self.initialize_base_classes()
            self.initialize_class_prerequisites()
            
            print("Classes system initialization completed successfully")
            
        except Exception as e:
            print(f"Error during initialization: {e}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    try:
        initializer = ClassesSystemInitializer()
        initializer.setup_classes_system()
    except Exception as e:
        print(f"Failed to initialize classes system: {e}")
        exit(1)