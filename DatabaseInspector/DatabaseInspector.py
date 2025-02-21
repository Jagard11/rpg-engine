# ./DatabaseInspector/DatabaseInspector.py

import streamlit as st
import sqlite3
import pandas as pd
from typing import List, Dict, Optional
import uuid

# --- Database Helper Functions ---
def get_tables() -> List[str]:
    """Get sorted list of all tables in the database."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
    tables = sorted(row[0] for row in cursor.fetchall())
    conn.close()
    return tables

def get_primary_key_column(table_name: str) -> Optional[str]:
    """Get the primary key column name for a table."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    cursor.execute(f"PRAGMA table_info({table_name});")
    for col in cursor.fetchall():
        if col[5]:  # Primary key flag
            conn.close()
            return col[1]
    conn.close()
    return None

def get_table_schema(table_name: str) -> pd.DataFrame:
    """Get schema as a DataFrame."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    cursor.execute(f"PRAGMA table_info({table_name});")
    schema = pd.DataFrame(
        cursor.fetchall(),
        columns=['cid', 'name', 'type', 'notnull', 'dflt_value', 'pk']
    )[['name', 'type', 'notnull', 'pk']]
    schema.columns = ['Column Name', 'Data Type', 'Not Null', 'Primary Key']
    conn.close()
    return schema

def get_table_contents(table_name: str) -> pd.DataFrame:
    """Get all records from a table as a DataFrame."""
    conn = sqlite3.connect('rpg_data.db')
    df = pd.read_sql_query(f"SELECT * FROM {table_name};", conn)
    conn.close()
    return df

def insert_record(table_name: str, data: dict) -> tuple:
    """Insert a new record into the table."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    columns = ', '.join(data.keys())
    placeholders = ', '.join(['?' for _ in data])
    try:
        cursor.execute(f"INSERT INTO {table_name} ({columns}) VALUES ({placeholders})", list(data.values()))
        conn.commit()
        conn.close()
        return True, "Record inserted"
    except Exception as e:
        conn.close()
        return False, str(e)

def update_record(table_name: str, record_id: int, data: dict) -> tuple:
    """Update an existing record."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    set_clause = ', '.join(f"{k} = ?" for k in data.keys())
    pk = get_primary_key_column(table_name) or 'id'
    try:
        cursor.execute(f"UPDATE {table_name} SET {set_clause} WHERE {pk} = ?", list(data.values()) + [record_id])
        conn.commit()
        conn.close()
        return True, "Record updated"
    except Exception as e:
        conn.close()
        return False, str(e)

def delete_record(table_name: str, record_id: int) -> tuple:
    """Delete a record."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    pk = get_primary_key_column(table_name) or 'id'
    try:
        cursor.execute(f"DELETE FROM {table_name} WHERE {pk} = ?", (record_id,))
        conn.commit()
        conn.close()
        return True, "Record deleted"
    except Exception as e:
        conn.close()
        return False, str(e)

def create_new_table(table_name: str, schema_df: pd.DataFrame) -> tuple:
    """Create a new table with the given schema."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    columns = []
    for _, row in schema_df.iterrows():
        if pd.isna(row['Column Name']) or not row['Column Name'].strip():
            continue
        col_def = f"{row['Column Name']} {row['Data Type']}"
        if row['Not Null']:
            col_def += " NOT NULL"
        if row['Primary Key']:
            col_def += " PRIMARY KEY AUTOINCREMENT"
        columns.append(col_def)
    if not columns:
        conn.close()
        return False, "No valid columns defined"
    try:
        cursor.execute(f"CREATE TABLE {table_name} ({', '.join(columns)});")
        conn.commit()
        conn.close()
        return True, "Table created successfully"
    except Exception as e:
        conn.close()
        return False, str(e)

# --- Change Tracking Functions ---
def compute_changes(original_df: pd.DataFrame, edited_df: pd.DataFrame, table_name: str) -> Dict:
    """Compute differences between original and edited dataframes."""
    pk = get_primary_key_column(table_name) or 'id'
    if not pk:
        raise ValueError("Table has no primary key")

    original_ids = set(original_df[pk].dropna().astype(int))
    edited_ids = set(edited_df[pk].dropna().astype(int))

    added_rows = edited_df[edited_df[pk].isin(edited_ids - original_ids)].to_dict('records')
    deleted_rows = original_df[original_df[pk].isin(original_ids - edited_ids)].to_dict('records')
    modified = {}
    for row_id in original_ids & edited_ids:
        orig = original_df[original_df[pk] == row_id].iloc[0]
        edit = edited_df[edited_df[pk] == row_id].iloc[0]
        diffs = {col: (orig[col], edit[col]) for col in original_df.columns if orig[col] != edit[col]}
        if diffs:
            modified[row_id] = diffs

    return {'added': added_rows, 'deleted': deleted_rows, 'modified': modified}

def apply_changes(table_name: str, change_set: Dict):
    """Apply changes to the database."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    pk = get_primary_key_column(table_name) or 'id'

    for row in change_set['added']:
        if not row.get(pk):
            continue
        columns = ', '.join(row.keys())
        placeholders = ', '.join(['?' for _ in row])
        cursor.execute(f"INSERT INTO {table_name} ({columns}) VALUES ({placeholders})", list(row.values()))

    for row in change_set['deleted']:
        cursor.execute(f"DELETE FROM {table_name} WHERE {pk} = ?", (row[pk],))

    for row_id, diffs in change_set['modified'].items():
        set_clause = ', '.join(f"{col} = ?" for col in diffs.keys())
        new_values = [val[1] for val in diffs.values()]
        cursor.execute(f"UPDATE {table_name} SET {set_clause} WHERE {pk} = ?", new_values + [row_id])

    conn.commit()
    conn.close()

def undo_changes(table_name: str, change_set: Dict):
    """Undo changes in the database."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    pk = get_primary_key_column(table_name) or 'id'

    for row in change_set['added']:
        cursor.execute(f"DELETE FROM {table_name} WHERE {pk} = ?", (row[pk],))

    for row in change_set['deleted']:
        columns = ', '.join(row.keys())
        placeholders = ', '.join(['?' for _ in row])
        cursor.execute(f"INSERT INTO {table_name} ({columns}) VALUES ({placeholders})", list(row.values()))

    for row_id, diffs in change_set['modified'].items():
        set_clause = ', '.join(f"{col} = ?" for col in diffs.keys())
        original_values = [val[0] for val in diffs.values()]
        cursor.execute(f"UPDATE {table_name} SET {set_clause} WHERE {pk} = ?", original_values + [row_id])

    conn.commit()
    conn.close()

def modify_table_schema(table_name: str, operations: List[Dict], column_mapping: Dict[str, str]) -> tuple:
    """Modify table schema with support for renames."""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    try:
        # Generate a unique temporary table name to avoid conflicts
        temp_table = f"temp_{table_name}_{uuid.uuid4().hex[:8]}"
        
        # Get current schema
        cursor.execute(f"PRAGMA table_info({table_name});")
        current_schema = {col[1]: col for col in cursor.fetchall()}
        new_columns, select_columns = [], []

        for col_name in current_schema:
            if col_name not in [op['column'] for op in operations if op['action'] == 'remove']:
                new_col_name = column_mapping.get(col_name, col_name)
                col = current_schema[col_name]
                modify_op = next((op for op in operations if op['action'] == 'modify' and op['column'] == new_col_name), None)
                if modify_op:
                    details = modify_op['details']
                    col_def = f"{new_col_name} {details['type']}"
                    if details.get('not_null'): col_def += " NOT NULL"
                    if details.get('primary_key'): col_def += " PRIMARY KEY AUTOINCREMENT"
                else:
                    col_def = f"{new_col_name} {col[2]}{' NOT NULL' if col[3] else ''}{' PRIMARY KEY AUTOINCREMENT' if col[5] else ''}"
                new_columns.append(col_def)
                select_columns.append(f"{col_name} AS {new_col_name}")

        for op in operations:
            if op['action'] == 'add':
                details = op['details']
                col_def = f"{op['column']} {details['type']}"
                if details.get('not_null'): col_def += " NOT NULL"
                if details.get('primary_key'): col_def += " PRIMARY KEY AUTOINCREMENT"
                new_columns.append(col_def)
                select_columns.append(f"NULL AS {op['column']}")

        # Create new table with modified schema
        cursor.execute(f"CREATE TABLE {temp_table} ({', '.join(new_columns)});")
        cursor.execute(f"INSERT INTO {temp_table} SELECT {', '.join(select_columns)} FROM {table_name};")
        cursor.execute(f"DROP TABLE {table_name};")
        cursor.execute(f"ALTER TABLE {temp_table} RENAME TO {table_name};")
        conn.commit()
        conn.close()
        return True, "Schema modified successfully"
    except Exception as e:
        conn.rollback()
        conn.close()
        return False, f"Error modifying schema: {str(e)}"

# --- UI Rendering ---
def render_db_inspector_tab():
    """Render the enhanced database inspector."""
    st.header("Database Inspector")
    inspector_tab, schema_tab = st.tabs(["Data Inspector", "Schema Editor"])

    with inspector_tab:
        render_data_inspector()

    with schema_tab:
        render_schema_editor()

def render_data_inspector():
    """Render data inspection and editing interface."""
    tables = get_tables()
    col1, col2 = st.columns([3, 1])
    with col1:
        selected_table = st.selectbox("Select Table", tables, key="data_table_select")
    with col2:
        if st.button("New Schema"):
            st.session_state['new_schema_mode'] = True
            st.session_state['selected_table'] = None

    if 'selected_table' not in st.session_state or st.session_state.get('selected_table') != selected_table:
        st.session_state['selected_table'] = selected_table
        st.session_state['undo_stack'] = []
        st.session_state['redo_stack'] = []
        st.session_state['original_df'] = get_table_contents(selected_table).copy()

    if selected_table and not st.session_state.get('new_schema_mode', False):
        st.subheader(f"Edit Data: {selected_table}")
        pk = get_primary_key_column(selected_table) or 'id'
        edited_df = st.data_editor(
            st.session_state['original_df'],
            num_rows="dynamic",
            column_config={
                pk: st.column_config.NumberColumn(required=True, min_value=1)
            }
        )

        col1, col2, col3 = st.columns(3)
        with col1:
            if st.button("Save Changes"):
                change_set = compute_changes(st.session_state['original_df'], edited_df, selected_table)
                apply_changes(selected_table, change_set)
                st.session_state['undo_stack'].append(change_set)
                st.session_state['redo_stack'] = []
                st.session_state['original_df'] = edited_df.copy()
                st.success("Changes saved!")
        with col2:
            if st.button("Undo"):
                if st.session_state['undo_stack']:
                    last_change = st.session_state['undo_stack'].pop()
                    undo_changes(selected_table, last_change)
                    st.session_state['redo_stack'].append(last_change)
                    st.session_state['original_df'] = get_table_contents(selected_table)
                    st.success("Undone last change")
                else:
                    st.warning("Nothing to undo")
        with col3:
            if st.button("Redo"):
                if st.session_state['redo_stack']:
                    next_change = st.session_state['redo_stack'].pop()
                    apply_changes(selected_table, next_change)
                    st.session_state['undo_stack'].append(next_change)
                    st.session_state['original_df'] = get_table_contents(selected_table)
                    st.success("Redone last change")
                else:
                    st.warning("Nothing to redo")

def render_schema_editor():
    """Render schema editing interface."""
    tables = get_tables()
    col1, col2 = st.columns([3, 1])
    with col1:
        selected_table = st.selectbox("Select Table", tables, key="schema_table_select")
    with col2:
        if st.button("New Schema", key="new_schema_btn"):
            st.session_state['new_schema_mode'] = True
            st.session_state['selected_table'] = None

    if st.session_state.get('new_schema_mode', False):
        st.subheader("Create New Schema")
        new_table_name = st.text_input("Table Name")
        if 'new_schema_df' not in st.session_state:
            st.session_state['new_schema_df'] = pd.DataFrame([{
                'Column Name': 'id',
                'Data Type': 'INTEGER',
                'Not Null': True,
                'Primary Key': True
            }])
        
        edited_schema_df = st.data_editor(
            st.session_state['new_schema_df'],
            num_rows="dynamic",
            column_config={
                'Column Name': st.column_config.TextColumn(required=True),
                'Data Type': st.column_config.SelectboxColumn(options=['INTEGER', 'TEXT', 'REAL', 'BLOB'], required=True)
            },
            column_order=['Column Name', 'Data Type', 'Not Null', 'Primary Key']
        )
        
        st.session_state['new_schema_df'] = edited_schema_df

        if st.button("Create Table"):
            if new_table_name and new_table_name not in tables:
                success, message = create_new_table(new_table_name, edited_schema_df)
                if success:
                    st.success(message)
                    st.session_state['new_schema_mode'] = False
                    st.session_state['selected_table'] = new_table_name
                    del st.session_state['new_schema_df']
                else:
                    st.error(message)
            else:
                st.error("Please provide a unique table name.")

    elif selected_table:
        st.subheader(f"Edit Schema: {selected_table}")
        schema_df = get_table_schema(selected_table)
        if 'edited_schema_df' not in st.session_state or st.session_state.get('edited_schema_table') != selected_table:
            st.session_state['edited_schema_df'] = schema_df.copy()
            st.session_state['edited_schema_table'] = selected_table

        edited_schema_df = st.data_editor(
            st.session_state['edited_schema_df'],
            num_rows="dynamic",
            column_config={
                'Column Name': st.column_config.TextColumn(required=True),
                'Data Type': st.column_config.SelectboxColumn(options=['INTEGER', 'TEXT', 'REAL', 'BLOB'], required=True)
            },
            column_order=['Column Name', 'Data Type', 'Not Null', 'Primary Key']
        )

        st.session_state['edited_schema_df'] = edited_schema_df

        st.warning("Note: Changing column types or removing columns may cause data loss.")
        if st.button("Apply Schema Changes"):
            operations, column_mapping = [], {}
            original_schema_df = schema_df.set_index('Column Name')
            edited_schema_df_clean = edited_schema_df.dropna(subset=['Column Name'])
            
            for _, row in edited_schema_df_clean.iterrows():
                if pd.isna(row['Column Name']) or not row['Column Name'].strip():
                    continue
                if row['Column Name'] in original_schema_df.index:
                    old_name = row['Column Name']
                    orig_row = original_schema_df.loc[old_name]
                    if any(orig_row[col] != row[col] for col in ['Data Type', 'Not Null', 'Primary Key']):
                        operations.append({
                            'action': 'modify',
                            'column': old_name,
                            'details': {
                                'type': row['Data Type'],
                                'not_null': row['Not Null'],
                                'primary_key': row['Primary Key']
                            }
                        })
                else:
                    operations.append({
                        'action': 'add',
                        'column': row['Column Name'],
                        'details': {
                            'type': row['Data Type'],
                            'not_null': row['Not Null'],
                            'primary_key': row['Primary Key']
                        }
                    })

            removed = set(original_schema_df.index) - set(edited_schema_df_clean['Column Name'])
            for col in removed:
                operations.append({'action': 'remove', 'column': col})

            success, message = modify_table_schema(selected_table, operations, column_mapping)
            if success:
                st.success(message)
                st.session_state['edited_schema_df'] = get_table_schema(selected_table)
                st.session_state['edited_schema_table'] = selected_table
            else:
                st.error(message)

if __name__ == "__main__":
    render_db_inspector_tab()