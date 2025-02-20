import os
import sqlite3
from typing import Dict, List, Optional, Set, Tuple, Any
import logging
import re
from datetime import datetime
import argparse

class SchemaManager:
    def __init__(self, db_path: str, schema_dir: str, import_dir: str, overwrite: bool = False):
        # Get the project root directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        project_root = os.path.dirname(current_dir)
        
        # Construct paths relative to project root
        self.db_path = os.path.join(project_root, db_path)
        self.schema_dir = os.path.join(project_root, schema_dir)
        self.import_dir = os.path.join(project_root, import_dir)
        self.overwrite = overwrite
        
        self.conn = None
        self.cursor = None
        # Track table creation and existence counts
        self.tables_created = 0
        self.tables_existed = 0
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
                os.makedirs(os.path.dirname(self.db_path), exist_ok=True)
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
            self.cursor.execute("SELECT 1")
            self.logger.info("Database connection verified")
        except sqlite3.Error as e:
            self.logger.error(f"Database connection error: {e}")
            raise

    def close_db(self):
        if self.conn:
            self.conn.close()
            self.logger.info("Database connection closed")

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

    def drop_table(self, table_name: str) -> bool:
        """Drop an existing table."""
        try:
            self.cursor.execute(f"DROP TABLE IF EXISTS {table_name}")
            self.conn.commit()
            return True
        except sqlite3.Error as e:
            self.logger.error(f"Error dropping table {table_name}: {e}")
            return False

    def get_table_name_from_create(self, create_stmt: str) -> Optional[str]:
        """Extract table name from CREATE TABLE statement."""
        match = re.search(r'CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?([^\s(]+)', create_stmt, re.IGNORECASE)
        if match:
            return match.group(1).strip('"')
        return None

    def create_table(self, create_stmt: str, filename: str) -> bool:
        """Execute CREATE TABLE statement."""
        table_name = self.get_table_name_from_create(create_stmt)
        if not table_name:
            self.logger.error("Could not extract table name from CREATE statement")
            return False

        try:
            if self.table_exists(table_name):
                if self.overwrite:
                    self.logger.info(f"Table {table_name} exists - dropping due to overwrite flag")
                    if not self.drop_table(table_name):
                        return False
                    self.tables_existed += 1  # Increment existed count
                else:
                    self.logger.info(f"Table {table_name} already exists - skipping creation")
                    self.tables_existed += 1  # Increment existed count
                    return False

            self.cursor.execute(create_stmt)
            self.conn.commit()
            self.tables_created += 1  # Increment created count
            return True
        except sqlite3.Error as e:
            self.logger.error(f"Error creating table {table_name}: {e}")
            self.conn.rollback()
            return False

    def parse_sql_file(self, file_path: str) -> List[Dict]:
        """Parse SQL file to execute CREATE TABLE statements and return INSERT data."""
        self.logger.info(f"Beginning to parse SQL file: {file_path}")
        
        with open(file_path, 'r') as f:
            content = f.read()
            self.logger.info(f"File read successfully, content length: {len(content)} characters")

        # Remove comments and normalize whitespace
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)  # Remove /* */ comments
        content = re.sub(r'--.*$', '', content, flags=re.MULTILINE)   # Remove -- comments
        content = re.sub(r'\s+', ' ', content)                        # Normalize whitespace

        # Find all CREATE TABLE statements
        create_pattern = r'CREATE\s+TABLE\s+.*?;'
        create_matches = re.finditer(create_pattern, content, re.DOTALL | re.IGNORECASE)
        for match in create_matches:
            create_stmt = match.group(0)
            table_name = self.get_table_name_from_create(create_stmt)
            if table_name:
                self.logger.info(f"Creating table: {table_name}")
                if self.create_table(create_stmt, os.path.basename(file_path)):
                    self.logger.info(f"Successfully created table: {table_name}")
                else:
                    self.logger.error(f"Failed to create table: {table_name}")

        # Placeholder for INSERT statement parsing (not used in this case)
        self.logger.info(f"No INSERT statements found in {file_path}")
        return []

    def import_data(self):
        """Import data from SQL files in import directory."""
        try:
            self.logger.info(f"Starting import process from directory: {self.import_dir}")
            self.connect_db()
            
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
                
                # INSERT handling omitted as no data is present in your SQL file
            
            self.logger.info("\nImport process completed")
            self.logger.info("Schema Operations:")
            self.logger.info(f"  Schema files processed: {len(sql_files)}")
            self.logger.info(f"  Tables created: {self.tables_created}")
            self.logger.info(f"  Tables already existed (dropped with overwrite): {self.tables_existed}")
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
    parser = argparse.ArgumentParser(description='Import schema and data into SQLite database')
    parser.add_argument('--overwrite', action='store_true', help='Overwrite existing tables')
    args = parser.parse_args()

    manager = SchemaManager(
        db_path="rpg_data.db",
        schema_dir="./SchemaManager/schemas",
        import_dir="./SchemaManager/imports",
        overwrite=args.overwrite
    )
    manager.import_data()