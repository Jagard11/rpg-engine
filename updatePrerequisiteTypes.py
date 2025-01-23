# updatePrerequisiteTypes.py

import sqlite3
from typing import List, Tuple

def execute_queries(queries: List[Tuple[str, tuple]]) -> None:
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    try:
        cursor.execute("BEGIN TRANSACTION")
        
        for query, params in queries:
            cursor.execute(query, params)
            
        cursor.execute("COMMIT")
        print("Database updated successfully")
        
    except Exception as e:
        cursor.execute("ROLLBACK")
        print(f"Error updating database: {str(e)}")
        raise
    finally:
        conn.close()

def main():
    queries = [
        # Drop existing constraint
        ("""
        SELECT sql FROM sqlite_master 
        WHERE type='table' AND name='class_prerequisites'
        """, ()),
        
        # Create temporary table
        ("""
        CREATE TABLE class_prerequisites_temp (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            class_id INTEGER NOT NULL,
            prerequisite_group INTEGER NOT NULL,
            prerequisite_type TEXT NOT NULL CHECK (
                prerequisite_type IN (
                    'specific_race', 'specific_job',
                    'race_category_total', 'job_category_total',
                    'race_subcategory_total', 'job_subcategory_total',
                    'karma', 'quest', 'achievement'
                )
            ),
            target_id INTEGER,
            required_level INTEGER,
            min_value INTEGER,
            max_value INTEGER,
            FOREIGN KEY (class_id) REFERENCES classes (id)
        )
        """, ()),
        
        # Copy data with type conversion
        ("""
        INSERT INTO class_prerequisites_temp (
            id, class_id, prerequisite_group, prerequisite_type,
            target_id, required_level, min_value, max_value
        )
        SELECT 
            id, class_id, prerequisite_group,
            CASE 
                WHEN prerequisite_type = 'specific_class' AND EXISTS (
                    SELECT 1 FROM classes 
                    WHERE classes.id = class_prerequisites.target_id 
                    AND classes.is_racial = TRUE
                ) THEN 'specific_race'
                WHEN prerequisite_type = 'specific_class' THEN 'specific_job'
                WHEN prerequisite_type = 'category_total' AND EXISTS (
                    SELECT 1 FROM classes 
                    WHERE classes.id = class_prerequisites.target_id 
                    AND classes.is_racial = TRUE
                ) THEN 'race_category_total'
                WHEN prerequisite_type = 'category_total' THEN 'job_category_total'
                WHEN prerequisite_type = 'subcategory_total' AND EXISTS (
                    SELECT 1 FROM classes 
                    WHERE classes.id = class_prerequisites.target_id 
                    AND classes.is_racial = TRUE
                ) THEN 'race_subcategory_total'
                WHEN prerequisite_type = 'subcategory_total' THEN 'job_subcategory_total'
                ELSE prerequisite_type
            END,
            target_id, required_level, min_value, max_value
        FROM class_prerequisites
        """, ()),
        
        # Drop old table
        ("DROP TABLE class_prerequisites", ()),
        
        # Rename new table
        ("ALTER TABLE class_prerequisites_temp RENAME TO class_prerequisites", ())
    ]
    
    execute_queries(queries)

if __name__ == "__main__":
    main()