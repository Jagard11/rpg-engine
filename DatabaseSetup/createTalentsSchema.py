# ./DatabaseSetup/createTalentsSchema.py

import sqlite3
import json
import os
from typing import List, Dict
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class TalentsSystemInitializer:
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
        """Create talent-related tables"""
        try:
            print("Creating talents system tables...")
            
            # Create talents table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS talents (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL UNIQUE,
                description TEXT,
                rarity INTEGER NOT NULL DEFAULT 200,
                is_combat_applicable BOOLEAN DEFAULT FALSE,
                discovery_threshold INTEGER,
                effect_type TEXT NOT NULL,
                effect_value TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """)

            # Create character_talents junction table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS character_talents (
                id INTEGER PRIMARY KEY,
                character_id INTEGER NOT NULL,
                talent_id INTEGER NOT NULL,
                is_discovered BOOLEAN DEFAULT FALSE,
                is_active BOOLEAN DEFAULT TRUE,
                discovery_condition_met BOOLEAN DEFAULT FALSE,
                discovered_at TIMESTAMP,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (character_id) REFERENCES characters(id),
                FOREIGN KEY (talent_id) REFERENCES talents(id)
            )
            """)

            # Create talent_conditions table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS talent_conditions (
                id INTEGER PRIMARY KEY,
                talent_id INTEGER NOT NULL,
                condition_type TEXT NOT NULL,
                condition_value TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (talent_id) REFERENCES talents(id)
            )
            """)

            # Create talent_effects table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS talent_effects (
                id INTEGER PRIMARY KEY,
                talent_id INTEGER NOT NULL,
                effect_type TEXT NOT NULL,
                effect_value TEXT NOT NULL,
                duration INTEGER,
                is_hidden BOOLEAN DEFAULT FALSE,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (talent_id) REFERENCES talents(id)
            )
            """)

            self.conn.commit()
            print("Talents system tables created successfully")
            return True
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error creating talents tables: {e}")
            return False

    def initialize_base_talents(self):
        """Initialize basic talents"""
        base_talents = [
            {
                'name': 'Natural Leader',
                'description': 'Innate ability to command and inspire others',
                'rarity': 200,
                'is_combat_applicable': True,
                'discovery_threshold': 10,
                'effect_type': 'leadership',
                'effect_value': 'allies_bonus:10',
                'conditions': [
                    {
                        'condition_type': 'charisma_check',
                        'condition_value': 'threshold:15'
                    }
                ],
                'effects': [
                    {
                        'effect_type': 'party_buff',
                        'effect_value': 'attack:5;defense:5',
                        'duration': 300,
                        'is_hidden': False
                    }
                ]
            },
            {
                'name': 'Magical Prodigy',
                'description': 'Extraordinary innate magical potential',
                'rarity': 200,
                'is_combat_applicable': True,
                'discovery_threshold': 15,
                'effect_type': 'magic_boost',
                'effect_value': 'spell_power:20',
                'conditions': [
                    {
                        'condition_type': 'cast_spell',
                        'condition_value': 'count:100'
                    }
                ],
                'effects': [
                    {
                        'effect_type': 'mana_regen',
                        'effect_value': 'rate:2',
                        'duration': 0,
                        'is_hidden': False
                    },
                    {
                        'effect_type': 'spell_power',
                        'effect_value': 'bonus:20',
                        'duration': 0,
                        'is_hidden': True
                    }
                ]
            },
            {
                'name': 'Blessed by Fortune',
                'description': 'Uncanny luck in critical situations',
                'rarity': 200,
                'is_combat_applicable': True,
                'discovery_threshold': 20,
                'effect_type': 'luck',
                'effect_value': 'critical_rate:5',
                'conditions': [
                    {
                        'condition_type': 'survive_fatal',
                        'condition_value': 'count:1'
                    }
                ],
                'effects': [
                    {
                        'effect_type': 'critical_chance',
                        'effect_value': 'bonus:5',
                        'duration': 0,
                        'is_hidden': False
                    },
                    {
                        'effect_type': 'dodge_chance',
                        'effect_value': 'bonus:3',
                        'duration': 0,
                        'is_hidden': True
                    }
                ]
            }
        ]

        try:
            for talent in base_talents:
                # Insert main talent entry
                self.cursor.execute("""
                INSERT INTO talents (
                    name, description, rarity, is_combat_applicable,
                    discovery_threshold, effect_type, effect_value
                ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """, (
                    talent['name'], talent['description'], talent['rarity'],
                    talent['is_combat_applicable'], talent['discovery_threshold'],
                    talent['effect_type'], talent['effect_value']
                ))
                
                talent_id = self.cursor.lastrowid
                
                # Insert talent conditions
                for condition in talent['conditions']:
                    self.cursor.execute("""
                    INSERT INTO talent_conditions (
                        talent_id, condition_type, condition_value
                    ) VALUES (?, ?, ?)
                    """, (talent_id, condition['condition_type'], condition['condition_value']))
                
                # Insert talent effects
                for effect in talent['effects']:
                    self.cursor.execute("""
                    INSERT INTO talent_effects (
                        talent_id, effect_type, effect_value, duration, is_hidden
                    ) VALUES (?, ?, ?, ?, ?)
                    """, (
                        talent_id, effect['effect_type'], effect['effect_value'],
                        effect['duration'], effect['is_hidden']
                    ))
            
            self.conn.commit()
            print("Base talents initialized successfully")
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error initializing base talents: {e}")
            raise

    def setup_talents_system(self):
        """Main method to set up the talents system"""
        try:
            self.connect_db()
            
            print("Starting talents system initialization...")
            
            if not self.create_tables():
                raise Exception("Failed to create required tables")
            
            self.initialize_base_talents()
            
            print("Talents system initialization completed successfully")
            
        except Exception as e:
            print(f"Error during initialization: {e}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    try:
        initializer = TalentsSystemInitializer()
        initializer.setup_talents_system()
    except Exception as e:
        print(f"Failed to initialize talents system: {e}")
        exit(1)