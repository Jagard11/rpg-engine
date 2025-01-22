# ./SchemaManager/TableCleanup.py

import sqlite3
import os
from pathlib import Path

def cleanup_database():
    # Get path to database in parent directory
    current_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(current_dir)
    db_path = os.path.join(parent_dir, 'rpg_data.db')
    
    print(f"Using database path: {db_path}")
    
    try:
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        print("Connected to database")

        # Get table info
        cursor.execute("PRAGMA table_info(character_classes)")
        columns = cursor.fetchall()
        print("\nCurrent character_classes columns:")
        for col in columns:
            print(f"  {col[1]} ({col[2]})")

        # Drop all existing tables to start fresh
        print("\nDropping existing tables...")
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
        tables = cursor.fetchall()
        
        # First drop tables with foreign key constraints
        for table in tables:
            try:
                print(f"Dropping table {table[0]}")
                cursor.execute(f"DROP TABLE IF EXISTS {table[0]}")
            except sqlite3.Error as e:
                print(f"Error dropping {table[0]}: {e}")

        # Also drop views
        print("\nDropping existing views...")
        cursor.execute("SELECT name FROM sqlite_master WHERE type='view'")
        views = cursor.fetchall()
        for view in views:
            try:
                print(f"Dropping view {view[0]}")
                cursor.execute(f"DROP VIEW IF EXISTS {view[0]}")
            except sqlite3.Error as e:
                print(f"Error dropping view {view[0]}: {e}")

        conn.commit()
        print("\nDatabase cleanup completed successfully")
        
    except sqlite3.Error as e:
        print(f"Database error: {e}")
    finally:
        conn.close()

def main():
    cleanup_database()

if __name__ == "__main__":
    main()