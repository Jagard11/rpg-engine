# ./SchemaManager/dumpSchemas.py

import sqlite3
import os
import json
from pathlib import Path
from typing import List, Dict, Any

class SchemaDumper:
    def __init__(self, db_path: str = 'rpg_data.db'):
        self.db_path = db_path
        self.output_dir = Path('./SchemaManager/schemas')
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
    def connect_db(self) -> sqlite3.Connection:
        return sqlite3.connect(self.db_path)
    
    def get_all_tables(self, cursor: sqlite3.Cursor) -> List[str]:
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
        return [row[0] for row in cursor.fetchall()]
    
    def get_table_info(self, cursor: sqlite3.Cursor, table: str) -> List[Dict[str, Any]]:
        cursor.execute(f"PRAGMA table_info({table})")
        return [dict(zip(['cid', 'name', 'type', 'notnull', 'dflt_value', 'pk'], row)) 
                for row in cursor.fetchall()]
    
    def get_table_triggers(self, cursor: sqlite3.Cursor, table: str) -> List[Dict[str, str]]:
        cursor.execute("""
            SELECT name, sql FROM sqlite_master 
            WHERE type='trigger' AND tbl_name=?
        """, (table,))
        return [{'name': row[0], 'sql': row[1]} for row in cursor.fetchall()]
    
    def get_table_indexes(self, cursor: sqlite3.Cursor, table: str) -> List[Dict[str, str]]:
        cursor.execute("""
            SELECT name, sql FROM sqlite_master 
            WHERE type='index' AND tbl_name=? AND name NOT LIKE 'sqlite_autoindex%'
        """, (table,))
        return [{'name': row[0], 'sql': row[1]} for row in cursor.fetchall()]
    
    def get_table_data(self, cursor: sqlite3.Cursor, table: str) -> List[Dict[str, Any]]:
        cursor.execute(f"SELECT * FROM {table}")
        columns = [desc[0] for desc in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    
    def generate_create_statement(self, table_info: List[Dict[str, Any]], table: str) -> str:
        columns = []
        for col in table_info:
            col_def = f"{col['name']} {col['type']}"
            if col['notnull']:
                col_def += " NOT NULL"
            if col['dflt_value'] is not None:
                col_def += f" DEFAULT {col['dflt_value']}"
            if col['pk']:
                col_def += " PRIMARY KEY"
            columns.append(col_def)
            
        return f"CREATE TABLE IF NOT EXISTS {table} (\n    " + ",\n    ".join(columns) + "\n)"
    
    def generate_schema_file(self, table: str, create_stmt: str, triggers: List[Dict], 
                           indexes: List[Dict], data: List[Dict]) -> str:
        schema_class = ''.join(word.capitalize() for word in table.split('_'))
        
        template = f'''# ./SchemaManager/schemas/create{schema_class}Schema.py

import sqlite3
import os
from pathlib import Path
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_{table}_schema():
    """Create the {table} table schema"""
    try:
        # Get path to database in parent directory
        current_dir = os.path.dirname(os.path.abspath(__file__))
        parent_dir = os.path.dirname(current_dir)
        db_path = os.path.join(parent_dir, 'rpg_data.db')
        
        logger.info(f"Creating {schema_class} schema in database: {{db_path}}")
        
        # Connect to database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON")
        
        # Create table
        cursor.execute("""
        {create_stmt}
        """)
        
        # Create triggers
'''
        
        # Add triggers
        for trigger in triggers:
            template += f'''        cursor.execute("""
        {trigger['sql']}
        """)\n\n'''
            
        # Add indexes  
        if indexes:
            template += '        # Create indexes\n'
            for index in indexes:
                template += f'''        cursor.execute("""
        {index['sql']}
        """)\n\n'''
                
        # Add data if exists
        if data:
            template += '        # Insert default data\n'
            template += '        default_data = [\n'
            for row in data:
                template += f'            {tuple(row.values())},\n'
            template += '        ]\n\n'
            
            placeholders = ','.join('?' * len(data[0]))
            columns = ','.join(data[0].keys())
            
            template += f'''        cursor.executemany("""
        INSERT OR IGNORE INTO {table} (
            {columns}            
        ) VALUES ({placeholders})
        """, default_data)\n'''
        
        template += '''        
        # Commit changes
        conn.commit()
        logger.info("Schema creation completed successfully.")
        
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
    create_{table}_schema()

if __name__ == "__main__":
    main()'''

        return template
    
    def dump_all_schemas(self):
        conn = self.connect_db()
        cursor = conn.cursor()
        
        tables = self.get_all_tables(cursor)
        for table in tables:
            if table == 'sqlite_sequence':
                continue
                
            table_info = self.get_table_info(cursor, table)
            create_stmt = self.generate_create_statement(table_info, table)
            triggers = self.get_table_triggers(cursor, table)
            indexes = self.get_table_indexes(cursor, table)
            data = self.get_table_data(cursor, table)
            
            schema_content = self.generate_schema_file(table, create_stmt, triggers, indexes, data)
            
            schema_class = ''.join(word.capitalize() for word in table.split('_'))
            output_file = self.output_dir / f'create{schema_class}Schema.py'
            
            with open(output_file, 'w') as f:
                f.write(schema_content)
                
        conn.close()

if __name__ == "__main__":
    dumper = SchemaDumper()
    dumper.dump_all_schemas()