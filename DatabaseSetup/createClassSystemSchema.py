# ./DatabaseSetup/createClassSystemSchema.py

import sqlite3
import json
import os
from typing import List, Dict
from pathlib import Path

class ClassSystemInitializer:
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
        """Create all necessary tables for the class system"""
        try:
            print("Creating database tables...")
            
            # Base character table
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS characters (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                level INTEGER NOT NULL DEFAULT 1,
                experience INTEGER NOT NULL DEFAULT 0,
                current_health INTEGER NOT NULL,
                max_health INTEGER NOT NULL,
                current_mana INTEGER NOT NULL,
                max_mana INTEGER NOT NULL,
                strength INTEGER NOT NULL,
                dexterity INTEGER NOT NULL,
                intelligence INTEGER NOT NULL,
                constitution INTEGER NOT NULL,
                description TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """)

            # Class system tables
            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS character_classes (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL UNIQUE,
                class_type TEXT CHECK(class_type IN ('common', 'rare', 'elite')) NOT NULL DEFAULT 'common',
                max_level INTEGER NOT NULL DEFAULT 15,
                description TEXT,
                stat_bonuses JSON,
                prerequisites JSON,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """)

            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS class_requirements (
                id INTEGER PRIMARY KEY,
                class_id INTEGER NOT NULL,
                required_class_id INTEGER NOT NULL,
                required_level INTEGER NOT NULL,
                FOREIGN KEY (class_id) REFERENCES character_classes(id),
                FOREIGN KEY (required_class_id) REFERENCES character_classes(id)
            )
            """)

            self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS character_class_levels (
                id INTEGER PRIMARY KEY,
                character_id INTEGER NOT NULL,
                class_id INTEGER NOT NULL,
                level INTEGER NOT NULL DEFAULT 1,
                experience_in_class INTEGER NOT NULL DEFAULT 0,
                unlocked_at_exp_level INTEGER NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (character_id) REFERENCES characters(id),
                FOREIGN KEY (class_id) REFERENCES character_classes(id),
                UNIQUE(character_id, class_id)
            )
            """)

            # Create triggers for class system
            self.cursor.execute("""
            CREATE TRIGGER IF NOT EXISTS enforce_class_max_level
            BEFORE INSERT ON character_class_levels
            FOR EACH ROW
            BEGIN
                SELECT CASE 
                    WHEN NEW.level > (
                        SELECT CASE class_type
                            WHEN 'common' THEN 15
                            WHEN 'rare' THEN 10
                            WHEN 'elite' THEN 5
                        END
                        FROM character_classes 
                        WHERE id = NEW.class_id
                    ) THEN
                        RAISE(ABORT, 'Level exceeds maximum allowed for class type')
                END;
            END;
            """)

            self.cursor.execute("""
            CREATE TRIGGER IF NOT EXISTS enforce_class_unlock_level
            BEFORE INSERT ON character_class_levels
            FOR EACH ROW
            BEGIN
                SELECT CASE 
                    WHEN NEW.unlocked_at_exp_level > (
                        SELECT level 
                        FROM characters 
                        WHERE id = NEW.character_id
                    ) THEN
                        RAISE(ABORT, 'Cannot unlock class at higher level than character level')
                END;
            END;
            """)

            # Create view for class summary
            self.cursor.execute("""
            CREATE VIEW IF NOT EXISTS character_class_summary AS
            SELECT 
                c.id as character_id,
                c.name as character_name,
                c.level as character_level,
                COUNT(ccl.id) as total_classes,
                SUM(CASE WHEN cc.class_type = 'common' THEN 1 ELSE 0 END) as common_classes,
                SUM(CASE WHEN cc.class_type = 'rare' THEN 1 ELSE 0 END) as rare_classes,
                SUM(CASE WHEN cc.class_type = 'elite' THEN 1 ELSE 0 END) as elite_classes
            FROM characters c
            LEFT JOIN character_class_levels ccl ON c.id = ccl.character_id
            LEFT JOIN character_classes cc ON ccl.class_id = cc.id
            GROUP BY c.id, c.name, c.level;
            """)

            self.conn.commit()
            print("Tables created successfully")
            return True
            
        except sqlite3.Error as e:
            self.conn.rollback()
            print(f"Error creating tables: {e}")
            return False

    def initialize_racial_classes(self):
        """Initialize racial class types"""
        racial_classes = [
            # Common Racial Classes (15 levels)
            {
                'name': 'Skeleton Mage',
                'description': 'A basic undead spellcaster, specializing in bone magic and necromancy',
                'class_type': 'common',
                'stat_bonuses': {
                    'intelligence': 2,
                    'wisdom': 1,
                    'necromantic_power': 1,
                    'mana_regeneration': 1
                },
                'requirements': []
            },
            # Rare Racial Classes (10 levels)
            {
                'name': 'Elder Lich',
                'description': 'An evolved undead mage with mastery over death magic',
                'class_type': 'rare',
                'stat_bonuses': {
                    'intelligence': 3,
                    'wisdom': 2,
                    'necromantic_power': 2,
                    'soul_capacity': 2
                },
                'requirements': [
                    {'class': 'Skeleton Mage', 'level': 10}
                ]
            },
            # Elite Racial Classes (5 levels)
            {
                'name': 'Overlord',
                'description': 'Supreme ruler of death with unmatched necromantic powers',
                'class_type': 'elite',
                'stat_bonuses': {
                    'intelligence': 4,
                    'wisdom': 3,
                    'necromantic_power': 3,
                    'leadership': 3,
                    'soul_mastery': 2
                },
                'requirements': [
                    {'class': 'Elder Lich', 'level': 8},
                    {'class': 'Skeleton Mage', 'level': 15}
                ]
            }
        ]

        for class_data in racial_classes:
            try:
                print(f"Adding racial class: {class_data['name']}")
                # Insert the class
                self.cursor.execute("""
                    INSERT INTO character_classes (
                        name, class_type, max_level, description, stat_bonuses
                    ) VALUES (?, ?, ?, ?, ?)
                """, (
                    class_data['name'],
                    class_data['class_type'],
                    15 if class_data['class_type'] == 'common' else 10 if class_data['class_type'] == 'rare' else 5,
                    class_data['description'],
                    json.dumps(class_data['stat_bonuses'])
                ))
                
                class_id = self.cursor.lastrowid
                
                # Insert requirements if any
                for req in class_data.get('requirements', []):
                    self.cursor.execute("""
                        INSERT INTO class_requirements (
                            class_id, required_class_id, required_level
                        ) VALUES (
                            ?,
                            (SELECT id FROM character_classes WHERE name = ?),
                            ?
                        )
                    """, (class_id, req['class'], req['level']))
                
                self.conn.commit()
                print(f"Successfully added racial class: {class_data['name']}")
                
            except sqlite3.Error as e:
                print(f"Error inserting {class_data['name']}: {e}")
                self.conn.rollback()
                raise

    def initialize_job_classes(self):
        """Initialize job class types"""
        job_classes = [
            # Common Job Classes (15 levels)
            {
                'name': 'Cook',
                'description': 'Master of culinary arts with magical cooking abilities',
                'class_type': 'common',
                'stat_bonuses': {
                    'constitution': 2,
                    'intelligence': 1,
                    'cooking_mastery': 2
                },
                'requirements': []
            },
            # Rare Job Classes (10 levels)
            {
                'name': 'Necromancer',
                'description': 'Professional practitioner of death magic',
                'class_type': 'rare',
                'stat_bonuses': {
                    'intelligence': 3,
                    'wisdom': 2,
                    'necromantic_power': 2
                },
                'requirements': [
                    {'class': 'Skeleton Mage', 'level': 5}
                ]
            },
            {
                'name': 'Master of Death',
                'description': 'Supreme authority in the arts of death and undeath',
                'class_type': 'rare',
                'stat_bonuses': {
                    'intelligence': 3,
                    'wisdom': 2,
                    'death_mastery': 3
                },
                'requirements': [
                    {'class': 'Necromancer', 'level': 7},
                    {'class': 'Elder Lich', 'level': 5}
                ]
            },
            # Elite Job Classes (5 levels)
            {
                'name': 'Eclipse',
                'description': 'Legendary death magic practitioner',
                'class_type': 'elite',
                'stat_bonuses': {
                    'intelligence': 4,
                    'wisdom': 3,
                    'death_mastery': 4,
                    'void_magic': 2
                },
                'requirements': [
                    {'class': 'Master of Death', 'level': 8},
                    {'class': 'Overlord', 'level': 3}
                ]
            }
        ]

        for class_data in job_classes:
            try:
                print(f"Adding job class: {class_data['name']}")
                # Insert the class
                self.cursor.execute("""
                    INSERT INTO character_classes (
                        name, class_type, max_level, description, stat_bonuses
                    ) VALUES (?, ?, ?, ?, ?)
                """, (
                    class_data['name'],
                    class_data['class_type'],
                    15 if class_data['class_type'] == 'common' else 10 if class_data['class_type'] == 'rare' else 5,
                    class_data['description'],
                    json.dumps(class_data['stat_bonuses'])
                ))
                
                class_id = self.cursor.lastrowid
                
                # Insert requirements if any
                for req in class_data.get('requirements', []):
                    self.cursor.execute("""
                        INSERT INTO class_requirements (
                            class_id, required_class_id, required_level
                        ) VALUES (
                            ?,
                            (SELECT id FROM character_classes WHERE name = ?),
                            ?
                        )
                    """, (class_id, req['class'], req['level']))
                
                self.conn.commit()
                print(f"Successfully added job class: {class_data['name']}")
                
            except sqlite3.Error as e:
                print(f"Error inserting {class_data['name']}: {e}")
                self.conn.rollback()
                raise

    def setup_class_system(self):
        """Main method to set up the entire class system"""
        try:
            self.connect_db()
            
            print("Starting class system initialization...")
            
            # First create all necessary tables
            if not self.create_tables():
                raise Exception("Failed to create required tables")
            
            # Initialize racial classes
            self.initialize_racial_classes()
            
            # Initialize job classes
            self.initialize_job_classes()
            
            print("Class system initialization completed successfully")
            
        except Exception as e:
            print(f"Error during initialization: {e}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    try:
        initializer = ClassSystemInitializer()
        initializer.setup_class_system()
    except Exception as e:
        print(f"Failed to initialize class system: {e}")
        exit(1)