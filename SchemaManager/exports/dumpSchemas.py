# ./SchemaManager/exports/dumpSchemas.py

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

def get_create_table_sql(conn, table_name):
    """Get the CREATE TABLE SQL statement for a given table"""
    cursor = conn.cursor()
    cursor.execute(
        "SELECT sql FROM sqlite_master WHERE type='table' AND name=?",
        (table_name,)
    )
    result = cursor.fetchone()
    return result[0] if result else None

def get_indexes(conn, table_name):
    """Get all indexes for a table"""
    cursor = conn.cursor()
    cursor.execute("""
        SELECT sql FROM sqlite_master 
        WHERE type='index' 
        AND tbl_name=? 
        AND sql IS NOT NULL
    """, (table_name,))
    return [row[0] for row in cursor.fetchall()]

def get_triggers(conn, table_name):
    """Get all triggers for a table"""
    cursor = conn.cursor()
    cursor.execute("""
        SELECT sql FROM sqlite_master 
        WHERE type='trigger' 
        AND tbl_name=? 
        AND sql IS NOT NULL
    """, (table_name,))
    return [row[0] for row in cursor.fetchall()]

def get_views(conn, table_name):
    """Get all views that reference this table"""
    cursor = conn.cursor()
    cursor.execute("""
        SELECT sql FROM sqlite_master 
        WHERE type='view' 
        AND sql LIKE ?
        AND sql IS NOT NULL
    """, (f'%{table_name}%',))
    return [row[0] for row in cursor.fetchall()]

def dump_table_schema(conn, table_name, output_dir):
    """Dump table schema files (structure, indexes, triggers, views)"""
    try:
        sanitized_table = sanitize_sql_identifier(table_name)
        if sanitized_table != table_name:
            logger.warning(f"Table name {table_name} contains unsafe characters")
            return

        cursor = conn.cursor()
        
        schema_dir = Path(output_dir)
        schema_dir.mkdir(parents=True, exist_ok=True)
        
        base_name = camel_case(table_name)
        structure_file = schema_dir / f"{base_name}Structure.sql"
        indexes_file = schema_dir / f"{base_name}Indexes.sql"
        triggers_file = schema_dir / f"{base_name}Triggers.sql"
        views_file = schema_dir / f"{base_name}Views.sql"

        create_sql = get_create_table_sql(conn, table_name)
        if not create_sql:
            logger.error(f"Could not get CREATE TABLE SQL for {table_name}")
            return

        indexes = get_indexes(conn, table_name)
        triggers = get_triggers(conn, table_name)
        views = get_views(conn, table_name)

        with open(structure_file, 'w') as f:
            f.write(f"-- ./SchemaManager/exports/schemas/{structure_file.name}\n")
            f.write(f"-- Generated: {datetime.now().isoformat()}\n\n")
            f.write(f"DROP TABLE IF EXISTS {table_name};\n")
            f.write(create_sql + ';\n')

        if indexes:
            with open(indexes_file, 'w') as f:
                f.write(f"-- ./SchemaManager/exports/schemas/{indexes_file.name}\n")
                f.write(f"-- Generated: {datetime.now().isoformat()}\n\n")
                for idx in indexes:
                    f.write(idx + ';\n')

        if triggers:
            with open(triggers_file, 'w') as f:
                f.write(f"-- ./SchemaManager/exports/schemas/{triggers_file.name}\n")
                f.write(f"-- Generated: {datetime.now().isoformat()}\n\n")
                for trigger in triggers:
                    f.write(trigger + ';\n')

        if views:
            with open(views_file, 'w') as f:
                f.write(f"-- ./SchemaManager/exports/schemas/{views_file.name}\n")
                f.write(f"-- Generated: {datetime.now().isoformat()}\n\n")
                for view in views:
                    f.write(view + ';\n')

        logger.info(f"Successfully dumped schema for table {table_name}")

    except Exception as e:
        logger.error(f"Error dumping schema for table {table_name}: {str(e)}")
        raise

def dump_all_schemas(db_path, output_dir):
    """Dump all table schemas from database"""
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
            dump_table_schema(conn, table_name, output_dir)
        
        logger.info("Schema dump completed successfully")
        
    except Exception as e:
        logger.error(f"Error during schema dump: {str(e)}")
        raise
    
    finally:
        if conn:
            conn.close()

if __name__ == '__main__':
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent
    
    DB_PATH = project_root / 'rpg_data.db'
    OUTPUT_DIR = script_dir / 'schemas'
    
    try:
        logger.info(f"Starting schema dump from {DB_PATH}")
        dump_all_schemas(DB_PATH, OUTPUT_DIR)
        logger.info("Schema dump process completed")
    except Exception as e:
        logger.error(f"Schema dump failed: {str(e)}")
        exit(1)