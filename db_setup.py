import sqlite3
from pathlib import Path

def init_database():
    """Initialize the SQLite database with all necessary tables"""
    db_path = Path("rpg_data.db")
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Create abilities table
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS abilities (
        id INTEGER PRIMARY KEY,
        name TEXT UNIQUE NOT NULL,
        description TEXT NOT NULL,
        ability_type TEXT NOT NULL,  -- 'spell', 'physical', 'buff', 'debuff'
        prerequisites TEXT,  -- JSON string of requirements
        mana_cost INTEGER,
        stamina_cost INTEGER,
        cooldown INTEGER,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
    ''')

    # Create characters table
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS characters (
        id INTEGER PRIMARY KEY,
        name TEXT UNIQUE NOT NULL,
        level INTEGER NOT NULL,
        class TEXT NOT NULL,
        race TEXT NOT NULL,
        health INTEGER NOT NULL,
        mana INTEGER NOT NULL,
        strength INTEGER NOT NULL,
        dexterity INTEGER NOT NULL,
        intelligence INTEGER NOT NULL,
        constitution INTEGER NOT NULL,
        description TEXT,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
    ''')

    # Create character_abilities junction table
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS character_abilities (
        character_id INTEGER,
        ability_id INTEGER,
        learned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (character_id) REFERENCES characters (id),
        FOREIGN KEY (ability_id) REFERENCES abilities (id),
        PRIMARY KEY (character_id, ability_id)
    )
    ''')

    # Insert some example abilities
    example_abilities = [
        ('Open Hand Strike', 'A precise strike using an open palm', 'physical', 
         '{"class": ["monk"], "level": 1, "dexterity": 12}', 0, 10, 0),
        ('Fireball', 'Launches a ball of fire at the target', 'spell',
         '{"class": ["mage", "sorcerer"], "level": 3, "intelligence": 14}', 30, 0, 6),
        ('Battle Focus', 'Increases accuracy and critical chance', 'buff',
         '{"class": ["monk", "fighter"], "level": 2, "wisdom": 12}', 20, 0, 12)
    ]
    
    cursor.executemany('''
    INSERT OR IGNORE INTO abilities (name, description, ability_type, prerequisites, 
                                   mana_cost, stamina_cost, cooldown)
    VALUES (?, ?, ?, ?, ?, ?, ?)
    ''', example_abilities)

    # Insert example character
    cursor.execute('''
    INSERT OR IGNORE INTO characters 
    (name, level, class, race, health, mana, strength, dexterity, intelligence, constitution, description)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ''', ('Kai', 3, 'monk', 'human', 100, 50, 14, 16, 12, 14, 
          'A disciplined monk focused on martial arts mastery'))

    # Get IDs for example data
    cursor.execute('SELECT id FROM characters WHERE name = ?', ('Kai',))
    char_id = cursor.fetchone()[0]
    
    cursor.execute('SELECT id FROM abilities WHERE name = ?', ('Open Hand Strike',))
    ability_id = cursor.fetchone()[0]

    # Link character to ability
    cursor.execute('''
    INSERT OR IGNORE INTO character_abilities (character_id, ability_id)
    VALUES (?, ?)
    ''', (char_id, ability_id))

    conn.commit()
    conn.close()

if __name__ == "__main__":
    init_database()