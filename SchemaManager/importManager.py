# ./SchemaManager/importManager.py

import os
import sqlite3
from typing import Dict, List, Optional, Set, Tuple, Any
import logging
import re
from datetime import datetime

class SchemaManager:
    def __init__(self, db_path: str, schema_dir: str, import_dir: str):
        self.db_path = os.path.join(os.getcwd(), db_path)  # Force absolute path
        self.schema_dir = schema_dir
        self.import_dir = import_dir
        self.conn = None
        self.cursor = None
        self.setup_logging()

    def setup_logging(self):
        """Set up logging with more detailed formatting"""
        logging.basicConfig(
            level=logging.DEBUG,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.StreamHandler(),
                logging.FileHandler('import_log.txt')
            ]
        )
        self.logger = logging.getLogger(__name__)

    def inspect_table_schema(self, table_name: str):
        """Inspect and log detailed table schema"""
        try:
            # Get table info
            self.cursor.execute(f"PRAGMA table_info({table_name})")
            columns = self.cursor.fetchall()
            self.logger.info(f"\nSchema for table '{table_name}':")
            for col in columns:
                self.logger.info(f"Column: {col[1]}, Type: {col[2]}, NotNull: {col[3]}, DefaultVal: {col[4]}, PK: {col[5]}")
            
            # Get sample data if available
            try:
                self.cursor.execute(f"SELECT * FROM {table_name} LIMIT 1")
                sample = self.cursor.fetchone()
                if sample:
                    self.logger.info(f"Sample record: {sample}")
            except sqlite3.Error as e:
                self.logger.error(f"Error getting sample data: {e}")
                
        except sqlite3.Error as e:
            self.logger.error(f"Error inspecting table schema: {e}")

    def verify_database_path(self):
        """Verify the database file exists"""
        if not os.path.exists(self.db_path):
            self.logger.error(f"Database file not found: {self.db_path}")
            self.logger.info(f"Current working directory: {os.getcwd()}")
            self.logger.info(f"Looking for database in: {os.path.abspath(self.db_path)}")
            raise FileNotFoundError(f"Database file not found: {self.db_path}")
        else:
            self.logger.info(f"Found database at: {self.db_path}")
            self.logger.info(f"Database size: {os.path.getsize(self.db_path)} bytes")

    def get_table_names(self) -> List[str]:
        """Get list of all tables in the database"""
        try:
            self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
            tables = [row[0] for row in self.cursor.fetchall()]
            self.logger.info(f"Found tables in database: {tables}")
            
            # Inspect schema for each table
            for table in tables:
                self.inspect_table_schema(table)
                
            return tables
        except sqlite3.Error as e:
            self.logger.error(f"Error getting table names: {e}")
            return []

    def verify_table_exists(self, table_name: str) -> bool:
        """Verify if a specific table exists in the database"""
        try:
            self.cursor.execute(f"SELECT name FROM sqlite_master WHERE type='table' AND name=?;", (table_name,))
            exists = bool(self.cursor.fetchone())
            if exists:
                self.logger.info(f"Found table: {table_name}")
                self.inspect_table_schema(table_name)
            else:
                self.logger.error(f"Table '{table_name}' not found in database. Available tables: {self.get_table_names()}")
            return exists
        except sqlite3.Error as e:
            self.logger.error(f"Error verifying table {table_name}: {e}")
            return False

    def connect_db(self):
        """Connect to the database and verify connection"""
        try:
            self.verify_database_path()
            self.logger.info(f"Attempting to connect to database: {self.db_path}")
            self.conn = sqlite3.connect(self.db_path)
            self.cursor = self.conn.cursor()
            self.logger.info("Database connection successful")
            
            # Get and log available tables
            tables = self.get_table_names()
            self.logger.info(f"Available tables in database: {tables}")
            
            # Verify database connection
            self.cursor.execute("SELECT 1")
            self.logger.info("Database connection verified")
        except sqlite3.Error as e:
            self.logger.error(f"Database connection error: {e}")
            raise

    def close_db(self):
        if self.conn:
            self.conn.close()
            self.logger.info("Database connection closed")

    def get_existing_records(self, table: str, columns: List[str]) -> Set[Tuple]:
        """Get existing records from a table for duplicate checking."""
        try:
            if not self.verify_table_exists(table):
                return set()
                
            columns_str = ', '.join(columns)
            self.logger.debug(f"Fetching existing records from {table} with columns: {columns_str}")
            self.cursor.execute(f"SELECT {columns_str} FROM {table}")
            records = {tuple(row) for row in self.cursor.fetchall()}
            self.logger.info(f"Found {len(records)} existing records in {table}")
            return records
        except sqlite3.Error as e:
            self.logger.error(f"Error fetching records from {table}: {e}")
            return set()

    def parse_values(self, values_str: str) -> List[str]:
        """Parse a single VALUE set into individual values."""
        values = []
        current_value = ''
        in_string = False
        paren_count = 0
        
        for char in values_str + ',':  # Add comma to handle last value
            if char == "'" and not in_string:
                in_string = True
                current_value += char
            elif char == "'" and in_string:
                in_string = False
                current_value += char
            elif char == '(' and not in_string:
                paren_count += 1
                if paren_count > 1:
                    current_value += char
            elif char == ')' and not in_string:
                paren_count -= 1
                if paren_count >= 1:
                    current_value += char
            elif char == ',' and not in_string and paren_count == 0:
                values.append(current_value.strip())
                current_value = ''
            else:
                current_value += char
                
        return [v for v in values if v]

    def process_value(self, value: str) -> Any:
        """Process a single value into its appropriate type."""
        value = value.strip()
        
        if value.upper() == 'CURRENT_TIMESTAMP':
            return datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        elif value.startswith("'") and value.endswith("'"):
            return value[1:-1]
        elif value.isdigit():
            return int(value)
        elif value.replace('.', '').isdigit() and value.count('.') == 1:
            return float(value)
        elif value.upper() in ('TRUE', 'FALSE'):
            return 1 if value.upper() == 'TRUE' else 0
        elif value.upper() == 'NULL':
            return None
        return value

    def parse_sql_file(self, file_path: str) -> List[Dict]:
        """Parse SQL INSERT statements into structured data."""
        self.logger.info(f"Beginning to parse SQL file: {file_path}")
        
        with open(file_path, 'r') as f:
            content = f.read()
            self.logger.info(f"File read successfully, content length: {len(content)} characters")
            self.logger.debug(f"File content preview: {content[:500]}...")

        # First clean up the content
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)  # Remove /* */ comments
        content = re.sub(r'--.*$', '', content, flags=re.MULTILINE)   # Remove -- comments
        content = re.sub(r'\s+', ' ', content)                        # Normalize whitespace
        
        # Extract table name and columns
        insert_match = re.search(r'INSERT\s+INTO\s+(\w+)\s*\(([\s\S]*?)\)\s*VALUES?\s*', content, re.IGNORECASE)
        if not insert_match:
            self.logger.error(f"No valid INSERT statement found in {file_path}")
            self.logger.debug(f"Content after cleanup: {content}")
            return []

        table_name = insert_match.group(1)
        columns_str = insert_match.group(2)
        columns = [col.strip() for col in columns_str.split(',')]
        
        self.logger.info(f"Found INSERT statement for table: {table_name}")
        self.logger.info(f"Columns identified: {', '.join(columns)}")

        # Verify table exists
        if not self.verify_table_exists(table_name):
            self.logger.error(f"Table '{table_name}' not found in database")
            return []

        # Find the VALUES section
        values_match = re.search(r'VALUES?\s*([\s\S]*);', content, re.IGNORECASE)
        if not values_match:
            self.logger.error("No VALUES clause found")
            return []
            
        values_section = values_match.group(1)
        self.logger.debug(f"Values section found: {values_section[:200]}...")

        # Split into individual value sets
        value_pattern = r'\(((?:[^()]|\([^()]*\))*)\)'
        value_matches = re.finditer(value_pattern, values_section)
        
        inserts = []
        for value_match in value_matches:
            values_str = value_match.group(1)
            self.logger.debug(f"Processing value set: {values_str}")
            
            # Parse the values
            raw_values = self.parse_values(values_str)
            processed_values = [self.process_value(v) for v in raw_values]
            
            if len(processed_values) == len(columns):
                record = {
                    'table': table_name,
                    'columns': columns,
                    'values': processed_values
                }
                self.logger.info(f"Parsed record: {dict(zip(columns, processed_values))}")
                inserts.append(record)
            else:
                self.logger.warning(
                    f"Column count mismatch: expected {len(columns)}, got {len(processed_values)}\n"
                    f"Columns: {columns}\n"
                    f"Values: {processed_values}"
                )

        self.logger.info(f"Successfully parsed {len(inserts)} records from {file_path}")
        return inserts

    def import_data(self):
        """Import data from SQL files in import directory."""
        try:
            self.logger.info(f"Starting import process from directory: {self.import_dir}")
            self.connect_db()
            
            # List and log all SQL files found
            sql_files = [f for f in os.listdir(self.import_dir) if f.endswith('.sql')]
            self.logger.info(f"Found {len(sql_files)} SQL files to process: {', '.join(sql_files)}")
            
            total_processed = 0
            total_success = 0
            total_duplicates = 0
            total_errors = 0
            
            for filename in sql_files:
                file_path = os.path.join(self.import_dir, filename)
                self.logger.info(f"\nProcessing file: {filename}")
                
                records = self.parse_sql_file(file_path)
                self.logger.info(f"Found {len(records)} records to import from {filename}")
                
                for record in records:
                    total_processed += 1
                    table = record['table']
                    
                    # Verify table exists before attempting to get existing records
                    if not self.verify_table_exists(table):
                        total_errors += 1
                        continue
                        
                    existing = self.get_existing_records(table, record['columns'])
                    
                    # Check for duplicates
                    values_tuple = tuple(record['values'])
                    if values_tuple in existing:
                        self.logger.info(
                            f"Skipping duplicate record:\n"
                            f"Table: {table}\n"
                            f"Values: {dict(zip(record['columns'], record['values']))}"
                        )
                        total_duplicates += 1
                        continue
                    
                    # Insert new record
                    placeholders = ','.join(['?' for _ in record['values']])
                    columns_str = ','.join(record['columns'])
                    sql = f"INSERT INTO {table} ({columns_str}) VALUES ({placeholders})"
                    
                    try:
                        self.logger.info(
                            f"Attempting to insert record:\n"
                            f"Table: {table}\n"
                            f"Values: {dict(zip(record['columns'], record['values']))}"
                        )
                        self.cursor.execute(sql, record['values'])
                        self.conn.commit()
                        total_success += 1
                        self.logger.info("Record inserted successfully")
                    except sqlite3.Error as e:
                        total_errors += 1
                        self.logger.error(
                            f"Error inserting record:\n"
                            f"Table: {table}\n"
                            f"Values: {dict(zip(record['columns'], record['values']))}\n"
                            f"Error: {e}"
                        )
                        self.conn.rollback()
            
            # Log final statistics
            self.logger.info("\nImport process completed")
            self.logger.info(f"Total records processed: {total_processed}")
            self.logger.info(f"Successfully inserted: {total_success}")
            self.logger.info(f"Duplicates skipped: {total_duplicates}")
            self.logger.info(f"Errors encountered: {total_errors}")
                        
        except Exception as e:
            self.logger.error(f"Import failed with error: {str(e)}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    manager = SchemaManager(
        db_path="rpg_data.db",  # Changed to game_data.db
        schema_dir="./SchemaManager/schemas",
        import_dir="./SchemaManager/imports"
    )
    manager.import_data()