# ./SchemaManager/dumpSchemas.py

import sqlite3
import os
from pathlib import Path
from typing import List, Dict, Any

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
                
    def get_index_list(self, cursor: sqlite3.Cursor, table: str) -> List[Dict[str, Any]]:
        cursor.execute(f"PRAGMA index_list({table})")
        return [dict(zip(['seq', 'name', 'unique', 'origin', 'partial'], row))
                for row in cursor.fetchall()]
                
    def get_index_info(self, cursor: sqlite3.Cursor, index_name: str) -> List[Dict[str, Any]]:
        cursor.execute(f"PRAGMA index_info({index_name})")
        return [dict(zip(['seqno', 'cid', 'name'], row))
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
    
    def generate_create_statement(self, table_info: List[Dict[str, Any]], 
                                foreign_keys: List[Dict[str, Any]], table: str,
                                cursor: sqlite3.Cursor) -> str:
        columns = []
        constraints = []
        
        for col in table_info:
            col_def = f"{col['name']} {col['type']}"
            if col['notnull']:
                col_def += " NOT NULL"
            if col['dflt_value'] is not None:
                col_def += f" DEFAULT {col['dflt_value']}"
            if col['pk']:
                col_def += " PRIMARY KEY"
            columns.append(col_def)
            
        for fk in foreign_keys:
            constraint = (f"FOREIGN KEY ({fk['from']}) REFERENCES {fk['table']}({fk['to']})")
            if fk['on_delete']:
                constraint += f" ON DELETE {fk['on_delete']}"
            if fk['on_update']:
                constraint += f" ON UPDATE {fk['on_update']}"
            constraints.append(constraint)
            
        # Get unique constraints from indexes
        index_list = self.get_index_list(cursor, table)
        for index in index_list:
            if index['unique'] and index['origin'] == 'u':  # origin 'u' means UNIQUE constraint
                index_info = self.get_index_info(cursor, index['name'])
                columns_in_constraint = [info['name'] for info in index_info]
                constraints.append(f"UNIQUE({', '.join(columns_in_constraint)})")
            
        all_statements = columns + constraints
        create_stmt = [f"        CREATE TABLE IF NOT EXISTS {table} ("]
        create_stmt.extend(f"            {stmt}," for stmt in all_statements[:-1])
        create_stmt.append(f"            {all_statements[-1]}")
        create_stmt.append("        )")
        
        return "\n".join(create_stmt)
    
    def format_sql(self, sql: str) -> str:
        """Format SQL with proper indentation for methods"""
        if not sql:
            return sql
        lines = sql.split('\n')
        formatted = []
        base_indent = 8  # 2 tabs for method scope
        
        for line in lines:
            stripped = line.strip()
            if stripped.startswith('CREATE'):
                formatted.append(" " * base_indent + stripped)
            elif stripped.startswith('BEGIN'):
                formatted.append(" " * base_indent + stripped)
                base_indent += 4
            elif stripped.startswith('END'):
                base_indent -= 4
                formatted.append(" " * base_indent + stripped)
            else:
                formatted.append(" " * (base_indent + 4) + stripped)
                
        return "\n".join(formatted)

    def generate_schema_file(self, table: str, create_stmt: str, triggers: List[Dict], 
                           indexes: List[Dict], data: List[Dict]) -> str:
        schema_class = ''.join(word.capitalize() for word in table.split('_'))
        
        template = [
            f'# ./SchemaManager/schemas/create{schema_class}Schema.py\n',
            'import sqlite3',
            'import os',
            'import logging',
            'from pathlib import Path\n',
            'logging.basicConfig(level=logging.INFO, format=\'%(asctime)s - %(levelname)s - %(message)s\')',
            'logger = logging.getLogger(__name__)\n',
            'def create_schema():',
            f'    """Create the {table} table schema"""',
            '    try:',
            '        # Get path to database in parent directory',
            '        current_dir = os.path.dirname(os.path.abspath(__file__))',
            '        parent_dir = os.path.dirname(current_dir)',
            '        db_path = os.path.join(parent_dir, \'rpg_data.db\')\n',
            f'        logger.info(f"Creating {schema_class} schema in database: {{db_path}}")\n',
            '        # Connect to database',
            '        conn = sqlite3.connect(db_path)',
            '        cursor = conn.cursor()\n',
            '        # Enable foreign keys',
            '        cursor.execute("PRAGMA foreign_keys = ON")\n',
            '        # Create table',
            '        cursor.execute("""\n' + create_stmt + '\n        """)\n'
        ]
        
        if triggers:
            template.append('        # Create triggers')
            for trigger in triggers:
                formatted_trigger = self.format_sql(trigger['sql'])
                template.append(f'        cursor.execute("""\n{formatted_trigger}\n        """)\n')
            
        if indexes:
            template.append('        # Create indexes')
            for index in indexes:
                formatted_index = self.format_sql(index['sql'])
                template.append(f'        cursor.execute("""\n{formatted_index}\n        """)\n')
                
        if data:
            template.append('        # Insert default data')
            template.append('        default_data = [')
            for row in data:
                template.append(f'            {tuple(row.values())},')
            template.append('        ]\n')
            
            columns = ','.join(data[0].keys())
            placeholders = ','.join(['?' for _ in range(len(data[0]))])
            
            template.extend([
                '        cursor.executemany("""',
                f'        INSERT OR IGNORE INTO {table} (',
                f'            {columns}',
                '        ) VALUES (' + placeholders + ')',
                '        """, default_data)\n'
            ])
        
        template.extend([
            '        # Commit changes',
            '        conn.commit()',
            '        logger.info("Schema creation completed successfully.")\n',
            '    except sqlite3.Error as e:',
            '        logger.error(f"SQLite error occurred: {str(e)}")',
            '        raise',
            '    except Exception as e:',
            '        logger.error(f"An error occurred: {str(e)}")',
            '        raise',
            '    finally:',
            '        if conn:',
            '            conn.close()',
            '            logger.info("Database connection closed.")\n',
            'if __name__ == "__main__":',
            '    create_schema()'
        ])
        
        return '\n'.join(template)
    
    def dump_all_schemas(self):
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        tables = self.get_all_tables(cursor)
        for table in tables:
            if table == 'sqlite_sequence':
                continue
                
            table_info = self.get_table_info(cursor, table)
            foreign_keys = self.get_foreign_keys(cursor, table)
            create_stmt = self.generate_create_statement(table_info, foreign_keys, table, cursor)
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