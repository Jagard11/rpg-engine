# ./setupDatabase.py

import sqlite3
import os
from pathlib import Path

def setup_database():
    """Create the RPG database and all required tables"""
    # Ensure we're in the correct directory
    db_path = Path('rpg_data.db')
    
    # If database exists, take a backup before proceeding
    if db_path.exists():
        backup_path = Path('rpg_data.db.backup')
        db_path.rename(backup_path)
        print(f"Created backup of existing database at {backup_path}")

    # Connect to database (creates it if it doesn't exist)
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()

    try:
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON;")

        # Create character_classes table
        cursor.execute("""
        CREATE TABLE character_classes (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            base_health INTEGER NOT NULL,
            base_mana INTEGER NOT NULL,
            base_strength INTEGER NOT NULL,
            base_dexterity INTEGER NOT NULL,
            base_intelligence INTEGER NOT NULL,
            base_constitution INTEGER NOT NULL
        );
        """)
        print("Created character_classes table")

        # Create abilities table
        cursor.execute("""
        CREATE TABLE abilities (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            description TEXT,
            mana_cost INTEGER,
            cooldown INTEGER,
            damage INTEGER,
            healing INTEGER,
            effects JSON,
            requirements JSON
        );
        """)
        print("Created abilities table")

        # Create class_abilities table
        cursor.execute("""
        CREATE TABLE class_abilities (
            class_id INTEGER,
            ability_id INTEGER,
            level_required INTEGER NOT NULL,
            FOREIGN KEY (class_id) REFERENCES character_classes (id),
            FOREIGN KEY (ability_id) REFERENCES abilities (id),
            PRIMARY KEY (class_id, ability_id)
        );
        """)
        print("Created class_abilities table")

        # Create characters table
        cursor.execute("""
        CREATE TABLE characters (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            class_id INTEGER NOT NULL,
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
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_id) REFERENCES character_classes (id)
        );
        """)
        print("Created characters table")

        # Create character_abilities table
        cursor.execute("""
        CREATE TABLE character_abilities (
            character_id INTEGER,
            ability_id INTEGER,
            unlocked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (character_id) REFERENCES characters (id),
            FOREIGN KEY (ability_id) REFERENCES abilities (id),
            PRIMARY KEY (character_id, ability_id)
        );
        """)
        print("Created character_abilities table")

        # Create items table if it doesn't exist
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS items (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            description TEXT,
            type TEXT NOT NULL,
            properties JSON
        );
        """)
        print("Created items table")

        # Create character_inventory table
        cursor.execute("""
        CREATE TABLE character_inventory (
            character_id INTEGER,
            item_id INTEGER,
            quantity INTEGER NOT NULL DEFAULT 1,
            equipped BOOLEAN NOT NULL DEFAULT FALSE,
            FOREIGN KEY (character_id) REFERENCES characters (id),
            FOREIGN KEY (item_id) REFERENCES items (id),
            PRIMARY KEY (character_id, item_id)
        );
        """)
        print("Created character_inventory table")

        # Create timestamp trigger for characters table
        cursor.execute("""
        CREATE TRIGGER update_character_timestamp 
        AFTER UPDATE ON characters
        BEGIN
            UPDATE characters 
            SET updated_at = CURRENT_TIMESTAMP 
            WHERE id = NEW.id;
        END;
        """)
        print("Created character update timestamp trigger")

        # Insert some initial character classes
        cursor.executemany(
            """
            INSERT INTO character_classes 
            (name, description, base_health, base_mana, base_strength, base_dexterity, base_intelligence, base_constitution)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """,
            [
                ('Warrior', 'A mighty melee fighter', 100, 50, 15, 10, 8, 12),
                ('Mage', 'A powerful spellcaster', 70, 120, 6, 8, 15, 8),
                ('Rogue', 'A cunning specialist', 80, 60, 10, 15, 10, 9),
                ('Cleric', 'A divine spellcaster', 90, 100, 8, 8, 12, 10)
            ]
        )
        print("Inserted initial character classes")

        # Insert some initial abilities
        cursor.executemany(
            """
            INSERT INTO abilities 
            (name, description, mana_cost, cooldown, damage, healing, effects)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
            [
                ('Slash', 'A basic sword attack', 0, 0, 10, 0, '{"type": "physical"}'),
                ('Fireball', 'A powerful fire spell', 30, 2, 25, 0, '{"type": "fire"}'),
                ('Heal', 'Restore health to target', 20, 1, 0, 15, '{"type": "healing"}'),
                ('Backstab', 'A sneaky attack from behind', 15, 3, 30, 0, '{"type": "physical"}')
            ]
        )
        print("Inserted initial abilities")

        # Link abilities to classes
        cursor.executemany(
            """
            INSERT INTO class_abilities 
            (class_id, ability_id, level_required)
            VALUES (?, ?, ?)
            """,
            [
                (1, 1, 1),  # Warrior - Slash
                (2, 2, 1),  # Mage - Fireball
                (3, 4, 1),  # Rogue - Backstab
                (4, 3, 1)   # Cleric - Heal
            ]
        )
        print("Linked initial abilities to classes")

        conn.commit()
        print("\nDatabase setup completed successfully!")

    except Exception as e:
        print(f"Error setting up database: {str(e)}")
        conn.rollback()
        # If there was an error and we had backed up the database, restore it
        if backup_path.exists():
            backup_path.rename(db_path)
            print("Restored database from backup due to error")
    finally:
        conn.close()

if __name__ == "__main__":
    setup_database()