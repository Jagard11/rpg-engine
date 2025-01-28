# ./SchemaManager/dumpSchemas.py

import sqlite3
from pathlib import Path
import re

def camel_case(s):
    """Convert string to CamelCase"""
    # Remove non-alphanumeric characters and split
    words = re.findall(r'[A-Za-z0-9]+', s)
    # Capitalize first letter of each word
    return ''.join(word.capitalize() for word in words)

def dump_table_schema(conn, table_name, output_dir):
    """Dump table schema and related SQL to separate files"""
    cursor = conn.cursor()
    
    # Get table creation SQL
    cursor.execute(f"SELECT sql FROM sqlite_master WHERE type='table' AND name=?", (table_name,))
    create_sql = cursor.fetchone()[0]
    
    # Get foreign key constraints
    cursor.execute("""
        SELECT sql FROM sqlite_master 
        WHERE type='table' AND name=? 
        AND sql LIKE '%FOREIGN KEY%'
    """, (table_name,))
    fk_constraints = cursor.fetchall()
    
    # Get table data
    cursor.execute(f"SELECT * FROM {table_name}")
    rows = cursor.fetchall()
    columns = [description[0] for description in cursor.description]
    
    # Prepare output directory
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Generate filenames using CamelCase
    base_name = camel_case(table_name)
    structure_file = output_dir / f"{base_name}Structure.sql"
    fk_file = output_dir / f"{base_name}ForeignKeys.sql"
    data_file = output_dir / f"{base_name}Data.sql"
    
    # Write structure
    with open(structure_file, 'w') as f:
        # Add filepath comment
        f.write(f"-- ./SchemaManager/schemas/{structure_file.name}\n\n")
        f.write(create_sql + ';\n')
    
    # Write foreign keys
    if fk_constraints:
        with open(fk_file, 'w') as f:
            # Add filepath comment
            f.write(f"-- ./SchemaManager/schemas/{fk_file.name}\n\n")
            for fk in fk_constraints:
                f.write(fk[0] + ';\n')
    
    # Write data
    if rows:
        with open(data_file, 'w') as f:
            # Add filepath comment
            f.write(f"-- ./SchemaManager/schemas/{data_file.name}\n\n")
            for row in rows:
                values = [
                    f"'{str(val).replace(chr(39), chr(39)+chr(39))}'" if isinstance(val, str)
                    else 'NULL' if val is None
                    else str(val)
                    for val in row
                ]
                f.write(f"INSERT INTO {table_name} ({', '.join(columns)}) VALUES ({', '.join(values)});\n")

def dump_all_schemas(db_path, output_dir):
    """Dump all table schemas from database"""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # Get all table names
    cursor.execute("""
        SELECT name FROM sqlite_master 
        WHERE type='table' 
        AND name NOT LIKE 'sqlite_%'
    """)
    tables = cursor.fetchall()
    
    for (table_name,) in tables:
        dump_table_schema(conn, table_name, output_dir)
    
    conn.close()

if __name__ == '__main__':
    db_path = 'rpg_data.db'
    output_dir = './SchemaManager/schemas'
    dump_all_schemas(db_path, output_dir)