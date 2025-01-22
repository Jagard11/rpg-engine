# ./SchemaManager/schemas/createClassExclusionSchema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_class_exclusion_schema():
    """Create the class exclusions table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating class exclusion schema in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create exclusions table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS class_exclusions (
            id INTEGER PRIMARY KEY,
            class_id INTEGER NOT NULL,
            exclusion_type TEXT NOT NULL CHECK(
                exclusion_type IN (
                    'specific_class',
                    'category_total',
                    'subcategory_total',
                    'racial_total',
                    'karma'
                )
            ),
            target_id INTEGER,
            min_value INTEGER,
            max_value INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (class_id) REFERENCES classes(id)
        )
        """)

        # Create trigger for updated_at
        cursor.execute("""
        CREATE TRIGGER IF NOT EXISTS update_exclusions_timestamp
        AFTER UPDATE ON class_exclusions
        BEGIN
            UPDATE class_exclusions 
            SET updated_at = CURRENT_TIMESTAMP 
            WHERE id = NEW.id;
        END;
        """)
        
        # Create trigger to validate target_id based on exclusion_type
        cursor.execute("""
        CREATE TRIGGER IF NOT EXISTS validate_exclusion_insert
        BEFORE INSERT ON class_exclusions
        FOR EACH ROW
        BEGIN
            SELECT CASE
                WHEN NEW.exclusion_type = 'specific_class' AND NOT EXISTS (SELECT 1 FROM classes WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid class reference in target_id')
                WHEN NEW.exclusion_type = 'category_total' AND NOT EXISTS (SELECT 1 FROM class_categories WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid category reference in target_id')
                WHEN NEW.exclusion_type = 'subcategory_total' AND NOT EXISTS (SELECT 1 FROM class_subcategories WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid subcategory reference in target_id')
                WHEN NEW.exclusion_type IN ('racial_total', 'karma') AND NEW.target_id IS NOT NULL
                    THEN RAISE(ABORT, 'target_id should be NULL for this exclusion type')
            END;
        END;
        """)
        
        cursor.execute("""
        CREATE TRIGGER IF NOT EXISTS validate_exclusion_update
        BEFORE UPDATE ON class_exclusions
        FOR EACH ROW
        BEGIN
            SELECT CASE
                WHEN NEW.exclusion_type = 'specific_class' AND NOT EXISTS (SELECT 1 FROM classes WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid class reference in target_id')
                WHEN NEW.exclusion_type = 'category_total' AND NOT EXISTS (SELECT 1 FROM class_categories WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid category reference in target_id')
                WHEN NEW.exclusion_type = 'subcategory_total' AND NOT EXISTS (SELECT 1 FROM class_subcategories WHERE id = NEW.target_id)
                    THEN RAISE(ABORT, 'Invalid subcategory reference in target_id')
                WHEN NEW.exclusion_type IN ('racial_total', 'karma') AND NEW.target_id IS NOT NULL
                    THEN RAISE(ABORT, 'target_id should be NULL for this exclusion type')
            END;
        END;
        """)
        
        # Create index for faster lookups
        cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_exclusions_class
        ON class_exclusions(class_id)
        """)
        
        # Commit changes
        conn.commit()
        logger.info("Class exclusion schema creation completed successfully.")
        
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
    create_class_exclusion_schema()

if __name__ == "__main__":
    main()