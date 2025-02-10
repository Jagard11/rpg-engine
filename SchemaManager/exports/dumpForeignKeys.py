# ./SchemaManager/exports/dumpForeignKeys.py

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

def get_foreign_keys(conn, table_name):
    """Get all foreign key constraints for a table"""
    cursor = conn.cursor()
    cursor.execute("""
        SELECT sql FROM sqlite_master 
        WHERE type='table' AND name=? 
        AND sql LIKE '%FOREIGN KEY%'
    """, (table_name,))
    return [row[0] for row in cursor.fetchall()]

def dump_table_foreign_keys(conn, table_name, output_dir):
    """Dump table foreign keys to SQL file"""
    try:
        sanitized_table = sanitize_sql_identifier(table_name)
        if sanitized_table != table_name:
            logger.warning(f"Table name {table_name} contains unsafe characters")
            return

        cursor = conn.cursor()
        
        fk_dir = Path(output_dir)
        fk_dir.mkdir(parents=True, exist_ok=True)
        
        base_name = camel_case(table_name)
        fk_file = fk_dir / f"{base_name}ForeignKeys.sql"

        foreign_keys = get_foreign_keys(conn, table_name)

        if foreign_keys:
            with open(fk_file, 'w') as f:
                f.write(f"-- ./SchemaManager/exports/foreign_keys/{fk_file.name}\n")
                f.write(f"-- Generated: {datetime.now().isoformat()}\n\n")
                for fk in foreign_keys:
                    f.write(fk + ';\n')

        logger.info(f"Successfully dumped foreign keys for table {table_name}")

    except Exception as e:
        logger.error(f"Error dumping foreign keys for table {table_name}: {str(e)}")
        raise

def dump_all_foreign_keys(db_path, output_dir):
    """Dump all table foreign keys from database"""
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
            dump_table_foreign_keys(conn, table_name, output_dir)
        
        logger.info("Foreign key dump completed successfully")
        
    except Exception as e:
        logger.error(f"Error during foreign key dump: {str(e)}")
        raise
    
    finally:
        if conn:
            conn.close()

if __name__ == '__main__':
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent
    
    DB_PATH = project_root / 'rpg_data.db'
    OUTPUT_DIR = script_dir / 'foreign_keys'
    
    try:
        logger.info(f"Starting foreign key dump from {DB_PATH}")
        dump_all_foreign_keys(DB_PATH, OUTPUT_DIR)
        logger.info("Foreign key dump process completed")
    except Exception as e:
        logger.error(f"Foreign key dump failed: {str(e)}")
        exit(1)