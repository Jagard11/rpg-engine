# ./SchemaManager/importManager.py

import os
import sqlite3
from typing import Dict, List, Optional, Set, Tuple
import logging
import re

class SchemaManager:
    def __init__(self, db_path: str, schema_dir: str, import_dir: str):
        self.db_path = db_path
        self.schema_dir = schema_dir
        self.import_dir = import_dir
        self.conn = None
        self.cursor = None
        self.setup_logging()

    def setup_logging(self):
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s'
        )
        self.logger = logging.getLogger(__name__)

    def connect_db(self):
        try:
            self.conn = sqlite3.connect(self.db_path)
            self.cursor = self.conn.cursor()
        except sqlite3.Error as e:
            self.logger.error(f"Database connection error: {e}")
            raise

    def close_db(self):
        if self.conn:
            self.conn.close()

    def get_existing_records(self, table: str) -> Set[Tuple]:
        """Get existing records from a table for duplicate checking."""
        try:
            self.cursor.execute(f"SELECT * FROM {table}")
            return {tuple(row) for row in self.cursor.fetchall()}
        except sqlite3.Error as e:
            self.logger.error(f"Error fetching records from {table}: {e}")
            return set()

    def parse_sql_file(self, file_path: str) -> List[Dict]:
        """Parse SQL INSERT statements into structured data."""
        with open(file_path, 'r') as f:
            content = f.read()

        # Extract table name from first INSERT statement
        table_match = re.search(r'INSERT INTO (\w+)', content)
        if not table_match:
            self.logger.error(f"No INSERT statement found in {file_path}")
            return []

        table_name = table_match.group(1)
        
        # Get column names from schema
        try:
            self.cursor.execute(f"PRAGMA table_info({table_name})")
            columns = [col[1] for col in self.cursor.fetchall()]
        except sqlite3.Error as e:
            self.logger.error(f"Error getting schema for {table_name}: {e}")
            return []

        # Parse INSERT statements
        inserts = []
        for statement in content.split(';'):
            if 'INSERT INTO' not in statement:
                continue
                
            values_match = re.search(r'VALUES\s*\((.*?)\)', statement, re.DOTALL)
            if values_match:
                values = values_match.group(1).split(',')
                values = [v.strip().strip("'") for v in values]
                
                if len(values) != len(columns):
                    self.logger.warning(f"Column count mismatch in {file_path}")
                    continue
                    
                record = {
                    'table': table_name,
                    'columns': columns,
                    'values': values
                }
                inserts.append(record)

        return inserts

    def import_data(self):
        """Import data from SQL files in import directory."""
        try:
            self.connect_db()
            
            for filename in os.listdir(self.import_dir):
                if not filename.endswith('.sql'):
                    continue
                    
                file_path = os.path.join(self.import_dir, filename)
                self.logger.info(f"Processing {filename}")
                
                records = self.parse_sql_file(file_path)
                for record in records:
                    table = record['table']
                    existing = self.get_existing_records(table)
                    
                    # Check for duplicates
                    values_tuple = tuple(record['values'])
                    if values_tuple in existing:
                        self.logger.info(f"Skipping duplicate record in {table}")
                        continue
                    
                    # Insert new record
                    placeholders = ','.join(['?' for _ in record['values']])
                    sql = f"INSERT INTO {table} ({','.join(record['columns'])}) VALUES ({placeholders})"
                    
                    try:
                        self.cursor.execute(sql, record['values'])
                        self.conn.commit()
                        self.logger.info(f"Inserted record into {table}")
                    except sqlite3.Error as e:
                        self.logger.error(f"Error inserting into {table}: {e}")
                        self.conn.rollback()
                        
        except Exception as e:
            self.logger.error(f"Import failed: {e}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    manager = SchemaManager(
        db_path="game.db",
        schema_dir="./SchemaManager/schemas",
        import_dir="./SchemaManager/imports"
    )
    manager.import_data()