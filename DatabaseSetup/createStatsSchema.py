# ./DatabaseSetup/createStatsSchema.py

import sqlite3
import json
import os
from typing import List, Dict
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class StatsSystemInitializer:
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
        """Create stats-related tables"""
        try:
            print("Creating stats system tables...")
            
            # Create base_stats table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS base_stats (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL UNIQUE,
                description TEXT,
                min_value INTEGER NOT NULL DEFAULT 0,
                max_value INTEGER NOT NULL DEFAULT 999,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """)

            # Create character_stats table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS character_stats (
                id INTEGER PRIMARY KEY,
                character_id INTEGER NOT NULL,
                stat_id INTEGER NOT NULL,
                base_value INTEGER NOT NULL,
                bonus_racial INTEGER DEFAULT 0,
                bonus_class INTEGER DEFAULT 0,
                bonus_equipment INTEGER DEFAULT 0,
                bonus_temporary INTEGER DEFAULT 0,
                last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (character_id) REFERENCES characters(id),
                FOREIGN KEY (stat_id) REFERENCES base_stats(id)
            )
            """)

            # Create stat_modifiers table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS stat_modifiers (
                id INTEGER PRIMARY KEY,
                character_id INTEGER NOT NULL,
                stat_id INTEGER NOT NULL,
                source_type TEXT NOT NULL,
                source_id INTEGER,
                modifier_value INTEGER NOT NULL,
                duration INTEGER,
                expires_at TIMESTAMP,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (character_id) REFERENCES characters(id),
                FOREIGN KEY (stat_id) REFERENCES base_stats(id)
            )
            """)

            # Create stat_scaling table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS stat_scaling (
                id INTEGER PRIMARY KEY,
                class_id INTEGER NOT NULL,
                stat_id INTEGER NOT NULL,
                level INTEGER NOT NULL,
                scaling_value REAL NOT NULL,
                FOREIGN KEY (class_id) REFERENCES character_classes(id),
                FOREIGN KEY (stat_id) REFERENCES base_stats(id)
            )
            """)

            self.conn.commit()
            print("Stats system tables created successfully")
            return True
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error creating stats tables: {e}")
            return False

    def initialize_base_stats(self):
        """Initialize basic stats"""
        base_stats = [
            ('Strength', 'Physical power and melee damage', 1, 999),
            ('Dexterity', 'Agility and precision', 1, 999),
            ('Constitution', 'Health and stamina', 1, 999),
            ('Intelligence', 'Magical power and knowledge', 1, 999),
            ('Wisdom', 'Spiritual power and insight', 1, 999),
            ('Charisma', 'Social influence and leadership', 1, 999),
            ('HP', 'Hit Points', 1, 9999),
            ('MP', 'Mana Points', 0, 9999),
            ('Physical Attack', 'Base physical damage', 1, 999),
            ('Physical Defense', 'Damage reduction', 1, 999),
            ('Magical Attack', 'Base magical damage', 1, 999),
            ('Magical Defense', 'Spell resistance', 1, 999)
        ]

        try:
            for stat in base_stats:
                self.cursor.execute("""
                INSERT OR IGNORE INTO base_stats (name, description, min_value, max_value)
                VALUES (?, ?, ?, ?)
                """, stat)
            
            self.conn.commit()
            print("Base stats initialized successfully")
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error initializing base stats: {e}")
            raise

    def setup_stats_system(self):
        """Main method to set up the stats system"""
        try:
            self.connect_db()
            
            print("Starting stats system initialization...")
            
            if not self.create_tables():
                raise Exception("Failed to create required tables")
            
            self.initialize_base_stats()
            
            print("Stats system initialization completed successfully")
            
        except Exception as e:
            print(f"Error during initialization: {e}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    try:
        initializer = StatsSystemInitializer()
        initializer.setup_stats_system()
    except Exception as e:
        print(f"Failed to initialize stats system: {e}")
        exit(1)