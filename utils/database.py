# ./utils/database.py

import sqlite3
from pathlib import Path

def get_db_connection():
    """Create a database connection to rpg_data.db."""
    db_path = Path("rpg_data.db")
    if not db_path.exists():
        raise FileNotFoundError("Database file not found at rpg_data.db")
    return sqlite3.connect(db_path)

def fetch_all(query: str, params: tuple = ()) -> list:
    """Execute a SELECT query and return all results."""
    conn = get_db_connection()
    try:
        cursor = conn.cursor()
        cursor.execute(query, params)
        columns = [desc[0] for desc in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def execute_transaction(query: str, params: tuple = ()) -> int:
    """Execute an INSERT/UPDATE/DELETE query with transaction support."""
    conn = get_db_connection()
    try:
        cursor = conn.cursor()
        cursor.execute("BEGIN TRANSACTION")
        cursor.execute(query, params)
        conn.commit()
        return cursor.lastrowid if "INSERT" in query.upper() else cursor.rowcount
    except Exception as e:
        conn.rollback()
        raise e
    finally:
        conn.close()