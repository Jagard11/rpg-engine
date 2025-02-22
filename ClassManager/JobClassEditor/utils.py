# ./ClassManager/JobClassEditor/utils.py

import sqlite3
import pandas as pd
from pathlib import Path

def get_db_connection():
    """Create a database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def get_foreign_key_options(table_name: str, name_field: str = 'name') -> dict[int, str]:
    """Get options for foreign key dropdown menus"""
    query = f"SELECT id, {name_field} FROM {table_name}"
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn)
            return dict(zip(df['id'], df[name_field]))
    except Exception as e:
        import streamlit as st
        st.error(f"Error loading {table_name}: {e}")
        return {}

def get_class_spell_schools(class_id: int) -> set:
    """Get magic schools from assigned spells"""
    query = """
    SELECT DISTINCT ms.name
    FROM class_spell_lists csl
    JOIN spells s ON csl.spell_id = s.id
    JOIN spell_has_effects she ON s.id = she.spell_id
    JOIN spell_effects se ON she.spell_effect_id = se.id
    JOIN magic_schools ms ON se.magic_school_id = ms.id
    WHERE csl.class_id = ?
    """
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn, params=[class_id])
            return set(df['name'])
    except Exception as e:
        import streamlit as st
        st.error(f"Error fetching spell schools: {e}")
        return set()