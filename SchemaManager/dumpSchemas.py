# ./SchemaManager/dumpSchemas.py

import sqlite3
import os
from pathlib import Path
from typing import List, Dict, Any
import json

class SchemaDumper:
    def __init__(self, db_path: str = 'rpg_data.db'):
        self.db_path = db_path
        self.output_dir = Path('./SchemaManager/schemas')
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
    def get_all_tables(self, cursor: sqlite3.Cursor) -> List[str]:
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
        return [row[0] for row in cursor.fetchall()]
    
    def get_table_info(self, cursor: sqlite3.Cursor, table: str) -> List[Dict[str, Any]]:
        cursor.execute(f"PRAGMA table_info({table})")
        return [dict(zip(['cid', 'name', 'type', 'notnull', 'dflt_value', 'pk'], row)) 
                for row in cursor.fetchall()]

    def get_foreign_keys(self, cursor: sqlite3.Cursor, table: str) -> List[Dict[str, Any]]:
        cursor.execute(f"PRAGMA foreign_key_list({table})")
        return [dict(zip(['id', 'seq', 'table', 'from', 'to', 'on_update', 'on_delete', 'match'], row))
                for row in cursor.fetchall()]
                
    def get_table_data(self, cursor: sqlite3.Cursor, table: str) -> List[Dict[str, Any]]:
        cursor.execute(f"SELECT * FROM {table}")
        columns = [desc[0] for desc in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]

    def dump_table_structure(self, table: str, table_info: List[Dict[str, Any]]) -> str:
        """Generate SQL structure file content"""
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
            
        create_stmt = [f"CREATE TABLE IF NOT EXISTS {table} ("]
        create_stmt.extend(f"    {stmt}," for stmt in columns[:-1])
        create_stmt.append(f"    {columns[-1]}")
        create_stmt.append(");")
        
        return "\n".join(create_stmt)

    def dump_all_schemas(self):
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        tables = self.get_all_tables(cursor)
        for table in tables:
            if table == 'sqlite_sequence':
                continue
                
            # Get table information
            table_info = self.get_table_info(cursor, table)
            foreign_keys = self.get_foreign_keys(cursor, table)
            table_data = self.get_table_data(cursor, table)
            
            # Generate filenames
            base_name = ''.join(word.capitalize() for word in table.split('_'))
            structure_file = self.output_dir / f'{base_name}Structure.sql'
            data_file = self.output_dir / f'{base_name}Data.sql'
            fk_file = self.output_dir / f'{base_name}ForeignKeys.sql'
            
            # Write structure file
            structure_content = self.dump_table_structure(table, table_info)
            with open(structure_file, 'w') as f:
                f.write(structure_content)
            
            # Write data file
            if table_data:
                data_content = []
                for record in table_data:
                    columns = ', '.join(record.keys())
                    values = ', '.join(
                        f"'{str(v)}'" if isinstance(v, str) else str(v) if v is not None else 'NULL'
                        for v in record.values()
                    )
                    data_content.append(f"INSERT INTO {table} ({columns}) VALUES ({values});")
                
                with open(data_file, 'w') as f:
                    f.write('\n'.join(data_content))
            
            # Write foreign keys file
            if foreign_keys:
                fk_content = []
                for fk in foreign_keys:
                    constraint = (
                        f"ALTER TABLE {table} ADD CONSTRAINT fk_{table}_{fk['from']}_{fk['table']} "
                        f"FOREIGN KEY ({fk['from']}) REFERENCES {fk['table']}({fk['to']})"
                    )
                    if fk['on_delete']:
                        constraint += f" ON DELETE {fk['on_delete']}"
                    if fk['on_update']:
                        constraint += f" ON UPDATE {fk['on_update']}"
                    constraint += ";"
                    fk_content.append(constraint)
                
                with open(fk_file, 'w') as f:
                    f.write('\n'.join(fk_content))
                
        conn.close()

if __name__ == "__main__":
    dumper = SchemaDumper()
    dumper.dump_all_schemas()