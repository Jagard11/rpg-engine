# ./SchemaManager/initializeSchema.py

import os
import sys
import sqlite3
import importlib.util
import logging
from pathlib import Path
from typing import List, Dict, Set, Tuple

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class SchemaInitializer:
    def __init__(self, db_path: str = 'rpg_data.db'):
        self.db_path = db_path
        self.current_dir = os.path.dirname(os.path.abspath(__file__))
        self.schemas_dir = os.path.join(self.current_dir, 'schemas')
        
    def get_create_table_sql(self, table_name: str) -> str:
        """Get CREATE TABLE SQL for an existing table"""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute(
                "SELECT sql FROM sqlite_master WHERE type='table' AND name=?", 
                (table_name,)
            )
            result = cursor.fetchone()
            return result[0] if result else None

    def parse_foreign_key(self, sql: str) -> Tuple[str, str]:
        """Parse foreign key constraint from ALTER TABLE statement"""
        table_name = sql.split('ALTER TABLE')[1].split('ADD')[0].strip()
        constraint = sql.split('FOREIGN KEY')[1].strip()
        return table_name, f"FOREIGN KEY {constraint}"

    def add_foreign_keys_to_table(self, table_name: str, constraints: List[str]) -> None:
        """Recreate table with foreign key constraints"""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            
            # Get original table creation SQL
            create_sql = self.get_create_table_sql(table_name)
            if not create_sql:
                raise Exception(f"Could not find CREATE TABLE SQL for {table_name}")

            # Create temporary table
            temp_name = f"temp_{table_name}"
            cursor.execute(f"CREATE TABLE {temp_name} AS SELECT * FROM {table_name}")
            
            # Drop original table
            cursor.execute(f"DROP TABLE {table_name}")
            
            # Create new table with constraints
            new_create_sql = create_sql.rstrip(')')
            for constraint in constraints:
                new_create_sql += f",\n    {constraint}"
            new_create_sql += "\n)"
            
            cursor.execute(new_create_sql)
            
            # Copy data back
            cursor.execute(f"INSERT INTO {table_name} SELECT * FROM {temp_name}")
            cursor.execute(f"DROP TABLE {temp_name}")
            
            conn.commit()

    def execute_sql_file(self, file_path: str) -> None:
        """Execute SQL statements from a file"""
        try:
            with open(file_path, 'r') as f:
                sql = f.read().strip()
                
                if 'ForeignKeys' in file_path:
                    statements = [s.strip() for s in sql.split(';') if s.strip()]
                    current_table = None
                    constraints = []
                    
                    for statement in statements:
                        try:
                            table_name, constraint = self.parse_foreign_key(statement)
                            
                            if current_table and table_name != current_table:
                                self.add_foreign_keys_to_table(current_table, constraints)
                                constraints = []
                            
                            current_table = table_name
                            constraints.append(constraint)
                            
                        except Exception as e:
                            logger.error(f"Error parsing foreign key statement: {statement}")
                            raise
                    
                    if current_table and constraints:
                        self.add_foreign_keys_to_table(current_table, constraints)
                
                else:
                    with sqlite3.connect(self.db_path) as conn:
                        conn.executescript(sql)
                        conn.commit()
                        
        except Exception as e:
            logger.error(f"Error executing {file_path}: {e}")
            raise

    def get_schema_files(self) -> Dict[str, List[str]]:
        """Get schema files grouped by type"""
        schema_files = {
            'structure': [],
            'data': [],
            'foreign_keys': []
        }
        
        for file in os.listdir(self.schemas_dir):
            if file.endswith('.sql'):
                if 'Structure' in file:
                    schema_files['structure'].append(file)
                elif 'Data' in file:
                    schema_files['data'].append(file)
                elif 'ForeignKeys' in file:
                    schema_files['foreign_keys'].append(file)
                    
        return schema_files

    def process_schema_files(self) -> None:
        """Process schema files in correct order"""
        schema_files = self.get_schema_files()
        
        # Run cleanup first
        cleanup_path = os.path.join(self.current_dir, 'TableCleanup.py')
        if os.path.exists(cleanup_path):
            module = self.import_module_from_file(cleanup_path)
            if hasattr(module, 'main'):
                module.main()

        # Process in order: structure -> data -> foreign keys
        for phase in ['structure', 'data', 'foreign_keys']:
            logger.info(f"\nProcessing {phase} files...")
            for file in sorted(schema_files[phase]):
                file_path = os.path.join(self.schemas_dir, file)
                logger.info(f"Executing {file}")
                self.execute_sql_file(file_path)

    @staticmethod
    def import_module_from_file(file_path: str):
        """Import a module from file path"""
        module_name = os.path.splitext(os.path.basename(file_path))[0]
        spec = importlib.util.spec_from_file_location(module_name, file_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

def main():
    initializer = SchemaInitializer()
    initializer.process_schema_files()

if __name__ == "__main__":
    main()