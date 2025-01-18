# ./DatabaseTimestampTriggers.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def get_tables_with_timestamps():
    """Get list of tables with created_at and/or updated_at columns"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    tables_with_timestamps = []
    
    try:
        # Get all tables
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
        tables = cursor.fetchall()
        
        for table in tables:
            table_name = table[0]
            # Get column info for each table
            cursor.execute(f"PRAGMA table_info({table_name});")
            columns = cursor.fetchall()
            
            # Check if table has timestamp columns
            has_created = any(col[1] == 'created_at' for col in columns)
            has_updated = any(col[1] == 'updated_at' for col in columns)
            
            if has_created or has_updated:
                tables_with_timestamps.append({
                    'name': table_name,
                    'has_created': has_created,
                    'has_updated': has_updated
                })
    
    finally:
        conn.close()
    
    return tables_with_timestamps

def create_timestamp_triggers():
    """Create triggers for automatic timestamp updates"""
    try:
        # Get path to database
        db_path = 'rpg_data.db'
        logger.info(f"Creating timestamp triggers in database: {db_path}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Get tables with timestamp columns
        tables = get_tables_with_timestamps()
        
        for table in tables:
            table_name = table['name']
            
            # Create insert trigger for created_at if exists
            if table['has_created']:
                trigger_name = f"{table_name}_insert_timestamp"
                cursor.execute(f"""
                DROP TRIGGER IF EXISTS {trigger_name};
                """)
                
                cursor.execute(f"""
                CREATE TRIGGER {trigger_name}
                AFTER INSERT ON {table_name}
                BEGIN
                    UPDATE {table_name}
                    SET created_at = DATETIME('now'),
                        updated_at = DATETIME('now')
                    WHERE rowid = NEW.rowid AND created_at IS NULL;
                END;
                """)
            
            # Create update trigger for updated_at if exists
            if table['has_updated']:
                trigger_name = f"{table_name}_update_timestamp"
                cursor.execute(f"""
                DROP TRIGGER IF EXISTS {trigger_name};
                """)
                
                cursor.execute(f"""
                CREATE TRIGGER {trigger_name}
                AFTER UPDATE ON {table_name}
                FOR EACH ROW
                BEGIN
                    UPDATE {table_name}
                    SET updated_at = DATETIME('now')
                    WHERE rowid = NEW.rowid;
                END;
                """)
        
        # Commit changes
        conn.commit()
        logger.info("Timestamp triggers created successfully.")
        
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
    """Entry point for trigger creation"""
    create_timestamp_triggers()

if __name__ == "__main__":
    main()