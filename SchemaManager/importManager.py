# ./SchemaManager/importManager.py

import os
import sqlite3
from typing import Dict, List, Optional, Set, Tuple, Any
import logging
import re
from datetime import datetime

class SchemaManager:
    def __init__(self, db_path: str, schema_dir: str, import_dir: str):
        # Get the project root directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(current_dir)
        
        # Construct paths relative to project root
        self.db_path = os.path.join(project_root, db_path)
        self.schema_dir = os.path.join(project_root, schema_dir)
        self.import_dir = os.path.join(project_root, import_dir)
        
        self.conn = None
        self.cursor = None
        # Track schema creation attempts
        self.schema_creation_attempts = {}
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

    def verify_database_path(self):
        """Verify the database file exists or create it if it doesn't"""
        if not os.path.exists(self.db_path):
            self.logger.warning(f"Database file not found: {self.db_path}")
            self.logger.info("Creating new database file")
            try:
                # Create the directory if it doesn't exist
                os.makedirs(os.path.dirname(self.db_path), exist_ok=True)
                # Create an empty database file
                conn = sqlite3.connect(self.db_path)
                conn.close()
                self.logger.info(f"Created new database at: {self.db_path}")
            except Exception as e:
                self.logger.error(f"Failed to create database: {e}")
                raise
        else:
            self.logger.info(f"Found existing database at: {self.db_path}")

    def connect_db(self):
        """Connect to the database and verify connection"""
        try:
            self.verify_database_path()
            self.logger.info(f"Attempting to connect to database: {self.db_path}")
            self.conn = sqlite3.connect(self.db_path)
            self.cursor = self.conn.cursor()
            self.logger.info("Database connection successful")
            
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

    def check_duplicate(self, table: str, name: str) -> bool:
        """Check if a record with the given name already exists"""
        try:
            self.cursor.execute(f"SELECT COUNT(*) FROM {table} WHERE name = ?", (name,))
            count = self.cursor.fetchone()[0]
            if count > 0:
                self.logger.info(f"Found existing record in {table} with name: {name}")
            return count > 0
        except sqlite3.Error as e:
            self.logger.error(f"Error checking for duplicate: {e}")
            return False

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

    def extract_create_table_statement(self, content: str) -> Optional[str]:
        """Extract CREATE TABLE statement from SQL content."""
        # Remove comments and normalize whitespace
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)  # Remove /* */ comments
        content = re.sub(r'--.*$', '', content, flags=re.MULTILINE)   # Remove -- comments
        content = content.strip()

        # Look for CREATE TABLE statement
        create_match = re.search(r'CREATE\s+TABLE\s+.*?;', content, re.DOTALL | re.IGNORECASE)
        if create_match:
            return create_match.group(0)
        return None

    def get_table_name_from_create(self, create_stmt: str) -> Optional[str]:
        """Extract table name from CREATE TABLE statement."""
        match = re.search(r'CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?([^\s(]+)', create_stmt, re.IGNORECASE)
        if match:
            return match.group(1)
        return None

    def table_exists(self, table_name: str) -> bool:
        """Check if a table already exists in the database."""
        try:
            self.cursor.execute("""
                SELECT name FROM sqlite_master 
                WHERE type='table' AND name=?
            """, (table_name,))
            return self.cursor.fetchone() is not None
        except sqlite3.Error as e:
            self.logger.error(f"Error checking table existence: {e}")
            return False

    def create_table(self, create_stmt: str, filename: str) -> bool:
        """Execute CREATE TABLE statement."""
        try:
            self.cursor.execute(create_stmt)
            self.conn.commit()
            self.schema_creation_attempts[filename] = True
            return True
        except sqlite3.Error as e:
            self.logger.error(f"Error creating table: {e}")
            self.conn.rollback()
            self.schema_creation_attempts[filename] = False
            return False

    def parse_sql_file(self, file_path: str) -> List[Dict]:
        """Parse SQL INSERT statements into structured data."""
        self.logger.info(f"Beginning to parse SQL file: {file_path}")
        
        with open(file_path, 'r') as f:
            content = f.read()
            self.logger.info(f"File read successfully, content length: {len(content)} characters")

        # Check for CREATE TABLE statement first
        create_stmt = self.extract_create_table_statement(content)
        if create_stmt:
            self.logger.info("Found CREATE TABLE statement")
            table_name = self.get_table_name_from_create(create_stmt)
            if table_name:
                self.logger.info(f"Creating table: {table_name}")
                if not self.table_exists(table_name):
                    if self.create_table(create_stmt, os.path.basename(file_path)):
                        self.logger.info(f"Successfully created table: {table_name}")
                    else:
                        self.logger.error(f"Failed to create table: {table_name}")
                else:
                    self.logger.info(f"Table {table_name} already exists")
                    self.schema_creation_attempts[os.path.basename(file_path)] = False

        # First clean up the content
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)  # Remove /* */ comments
        content = re.sub(r'--.*$', '', content, flags=re.MULTILINE)   # Remove -- comments
        content = re.sub(r'\s+', ' ', content)                        # Normalize whitespace
        
        # Extract INSERT statements
        insert_match = re.search(r'INSERT\s+INTO\s+(\w+)\s*\(([\s\S]*?)\)\s*VALUES?\s*', content, re.IGNORECASE)
        if not insert_match:
            self.logger.info(f"No INSERT statements found in {file_path}")
            return []

        table_name = insert_match.group(1)
        columns_str = insert_match.group(2)
        columns = [col.strip() for col in columns_str.split(',')]
        
        self.logger.info(f"Found INSERT statement for table: {table_name}")
        self.logger.info(f"Columns identified: {', '.join(columns)}")

        # Find the VALUES section
        values_match = re.search(r'VALUES?\s*([\s\S]*);', content, re.IGNORECASE)
        if not values_match:
            self.logger.info("No VALUES clause found")
            return []
            
        values_section = values_match.group(1)

        # Split into individual value sets
        value_pattern = r'\(((?:[^()]|\([^()]*\))*)\)'
        value_matches = re.finditer(value_pattern, values_section)
        
        inserts = []
        for value_match in value_matches:
            values_str = value_match.group(1)
            
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
                if not records:
                    self.logger.info(f"No INSERT records found in {filename} (may contain only CREATE TABLE)")
                    continue
                    
                self.logger.info(f"Found {len(records)} records to import from {filename}")
                
                for record in records:
                    total_processed += 1
                    table = record['table']
                    
                    # Get name value from record if it exists
                    try:
                        name_index = record['columns'].index('name')
                        name_value = record['values'][name_index]
                        
                        # Check for duplicates only if there's a name field
                        if self.check_duplicate(table, name_value):
                            self.logger.info(
                                f"Skipping duplicate record:\n"
                                f"Table: {table}\n"
                                f"Name: {name_value}"
                            )
                            total_duplicates += 1
                            continue
                    except ValueError:
                        # No name column, proceed without duplicate check
                        self.logger.info("No 'name' column found in record, skipping duplicate check")
                        pass
                    
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
            self.logger.info("Schema Operations:")
            self.logger.info(f"  Schema files processed: {len(sql_files)}")
            self.logger.info(f"  Tables created: {sum(1 for f in self.schema_creation_attempts if self.schema_creation_attempts[f])}")
            self.logger.info(f"  Tables already existed: {sum(1 for f in self.schema_creation_attempts if not self.schema_creation_attempts[f])}")
            self.logger.info("\nData Import Operations:")
            self.logger.info(f"  Total records processed: {total_processed}")
            self.logger.info(f"  Successfully inserted: {total_success}")
            self.logger.info(f"  Duplicates skipped: {total_duplicates}")
            self.logger.info(f"  Errors encountered: {total_errors}")
                        
        except Exception as e:
            self.logger.error(f"Import failed with error: {str(e)}")
            raise
        finally:
            self.close_db()

if __name__ == "__main__":
    manager = SchemaManager(
        db_path="rpg_data.db",
        schema_dir="./SchemaManager/schemas",
        import_dir="./SchemaManager/imports"
    )
    manager.import_data()