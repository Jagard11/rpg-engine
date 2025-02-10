# ./SchemaManager/exports/dumpData.py

import sqlite3
from pathlib import Path
import re
import logging
from datetime import datetime

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

def sanitize_sql_identifier(identifier):
    """Sanitize SQL identifiers to prevent SQL injection"""
    sanitized = re.sub(r'[^\w]', '', identifier)
    return sanitized

def camel_case(s):
    """Convert string to CamelCase"""
    words = re.findall(r'[A-Za-z0-9]+', s)
    return ''.join(word.capitalize() for word in words)

def format_value(val):
    """Format a value for SQL insertion"""
    if val is None:
        return 'NULL'
    elif isinstance(val, (int, float)):
        return str(val)
    elif isinstance(val, bool):
        return '1' if val else '0'
    elif isinstance(val, (bytes, bytearray)):
        return f"X'{val.hex()}'"
    else:
        return f"'{str(val).replace(chr(39), chr(39)+chr(39))}'"

def dump_table_data(conn, table_name, output_dir):
    """Dump table data to SQL file using a more efficient format"""
    try:
        sanitized_table = sanitize_sql_identifier(table_name)
        if sanitized_table != table_name:
            logger.warning(f"Table name {table_name} contains unsafe characters")
            return

        cursor = conn.cursor()
        
        data_dir = Path(output_dir)
        data_dir.mkdir(parents=True, exist_ok=True)
        
        base_name = camel_case(table_name)
        data_file = data_dir / f"{base_name}Data.sql"

        cursor.execute(f"SELECT * FROM {table_name}")
        rows = cursor.fetchall()
        columns = [description[0] for description in cursor.description]

        if rows:
            with open(data_file, 'w') as f:
                f.write(f"-- ./SchemaManager/exports/data/{data_file.name}\n")
                f.write(f"-- Generated: {datetime.now().isoformat()}\n\n")
                f.write(f"BEGIN TRANSACTION;\n\n")
                f.write(f"DELETE FROM {table_name};\n\n")
                
                # Write column definitions once
                f.write(f"INSERT INTO {table_name} ({', '.join(columns)})\nVALUES\n")
                
                # Write all values
                values_list = []
                for row in rows:
                    values = [format_value(val) for val in row]
                    values_list.append(f"({', '.join(values)})")
                
                # Join with commas and semicolon at the end
                f.write(',\n'.join(values_list))
                f.write(';\n\n')
                
                f.write("COMMIT;\n")

        logger.info(f"Successfully dumped data for table {table_name}")

    except Exception as e:
        logger.error(f"Error dumping data for table {table_name}: {str(e)}")
        raise

def dump_all_data(db_path, output_dir):
    """Dump all table data from database"""
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        
        cursor.execute("""
            SELECT name FROM sqlite_master 
            WHERE type='table' 
            AND name NOT LIKE 'sqlite_%'
            ORDER BY name
        """)
        tables = cursor.fetchall()
        
        if not tables:
            logger.warning("No tables found in database")
            return
        
        logger.info(f"Found {len(tables)} tables to process")
        
        for (table_name,) in tables:
            dump_table_data(conn, table_name, output_dir)
        
        logger.info("Data dump completed successfully")
        
    except Exception as e:
        logger.error(f"Error during data dump: {str(e)}")
        raise
    
    finally:
        if conn:
            conn.close()

if __name__ == '__main__':
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent
    
    DB_PATH = project_root / 'rpg_data.db'
    OUTPUT_DIR = script_dir / 'data'
    
    try:
        logger.info(f"Starting data dump from {DB_PATH}")
        dump_all_data(DB_PATH, OUTPUT_DIR)
        logger.info("Data dump process completed")
    except Exception as e:
        logger.error(f"Data dump failed: {str(e)}")
        exit(1)