# ./DatabaseSetup/createPrerequisiteSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_prerequisite_schema():
    """Create the class prerequisites table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating prerequisite schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create prerequisites table with prerequisite groups for OR conditions
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_prerequisites (
            id INTEGER PRIMARY KEY,
            class_id INTEGER NOT NULL,
            prerequisite_group INTEGER NOT NULL,
            prerequisite_type TEXT NOT NULL CHECK(
                prerequisite_type IN (
                    'specific_class',
                    'category_total', 
                    'subcategory_total',
                    'karma',
                    'quest',
                    'achievement'
                )
            ),
            target_id INTEGER,
            required_level INTEGER,
            min_value INTEGER,
            max_value INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_id) REFERENCES classes(id)
        )
        """)
        
        # Create trigger for updated_at
        cursor.execute("""
        CREATE TRIGGER IF NOT EXISTS update_prerequisites_timestamp
        AFTER UPDATE ON class_prerequisites
        BEGIN
            UPDATE class_prerequisites 
            SET updated_at = CURRENT_TIMESTAMP 
            WHERE id = NEW.id;
        END;
        """)
        
        # Create trigger to validate target_id based on prerequisite_type
        cursor.execute("""
        CREATE TRIGGER IF NOT EXISTS validate_prerequisite_insert
        BEFORE INSERT ON class_prerequisites
        FOR EACH ROW
        BEGIN
            SELECT CASE
                WHEN NEW.prerequisite_type = 'specific_class' AND NOT EXISTS (SELECT 1 FROM classes WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid class reference in target_id')
                WHEN NEW.prerequisite_type = 'category_total' AND NOT EXISTS (SELECT 1 FROM class_categories WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid category reference in target_id')
                WHEN NEW.prerequisite_type = 'subcategory_total' AND NOT EXISTS (SELECT 1 FROM class_subcategories WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid subcategory reference in target_id')
                WHEN NEW.prerequisite_type IN ('karma', 'quest', 'achievement') AND NEW.target_id IS NOT NULL
                    THEN RAISE(ABORT, 'target_id should be NULL for this prerequisite type')
            END;
        END;
        """)
        
        cursor.execute("""
        CREATE TRIGGER IF NOT EXISTS validate_prerequisite_update
        BEFORE UPDATE ON class_prerequisites
        FOR EACH ROW
        BEGIN
            SELECT CASE
                WHEN NEW.prerequisite_type = 'specific_class' AND NOT EXISTS (SELECT 1 FROM classes WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid class reference in target_id')
                WHEN NEW.prerequisite_type = 'category_total' AND NOT EXISTS (SELECT 1 FROM class_categories WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid category reference in target_id')
                WHEN NEW.prerequisite_type = 'subcategory_total' AND NOT EXISTS (SELECT 1 FROM class_subcategories WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid subcategory reference in target_id')
                WHEN NEW.prerequisite_type IN ('karma', 'quest', 'achievement') AND NEW.target_id IS NOT NULL
                    THEN RAISE(ABORT, 'target_id should be NULL for this prerequisite type')
            END;
        END;
        """)
        
        # Create index for faster lookups
        cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_prerequisites_class
        ON class_prerequisites(class_id)
        """)
        
        cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_prerequisites_group
        ON class_prerequisites(class_id, prerequisite_group)
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Prerequisite schema creation completed successfully.")
        
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

def main():
    """Entry point for schema creation"""
    create_prerequisite_schema()

if __name__ == "__main__":
    main()