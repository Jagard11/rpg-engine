# ./ClassManager/JobClassEditor/classesTable.py

import streamlit as st
import sqlite3
import pandas as pd
from pathlib import Path
from typing import Tuple

def get_db_connection():
    """Create a database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def load_job_classes(limit=25, offset=0):
    """Load job classes with limit and offset for pagination"""
    query = """
    SELECT id, name, class_type, category_id, subcategory_id
    FROM classes
    WHERE is_racial = 0
    LIMIT ? OFFSET ?
    """
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn, params=[limit, offset])
            return df
    except Exception as e:
        st.error(f"Error loading job classes: {e}")
        return pd.DataFrame()

def get_total_job_classes():
    """Get the total number of job classes"""
    query = "SELECT COUNT(*) FROM classes WHERE is_racial = 0"
    try:
        with get_db_connection() as conn:
            cursor = conn.execute(query)
            return cursor.fetchone()[0]
    except Exception as e:
        st.error(f"Error getting total job classes: {e}")
        return 0

def load_class_record(class_id):
    """Load a specific class record by ID"""
    query = "SELECT * FROM classes WHERE id = ?"
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn, params=[class_id])
            if not df.empty:
                return df.iloc[0].to_dict()
    except Exception as e:
        st.error(f"Error loading class record: {e}")
    return None

def save_class_record(record_data, is_new=True):
    """Save a class record to the database"""
    if is_new:
        columns = [k for k in record_data.keys() if k not in ['id', 'created_at', 'updated_at']]
        placeholders = ','.join(['?' for _ in columns])
        query = f"INSERT INTO classes ({','.join(columns)}) VALUES ({placeholders})"
        values = [record_data[col] for col in columns]
    else:
        columns = [k for k in record_data.keys() if k not in ['id', 'created_at', 'updated_at']]
        set_clause = ','.join([f"{col} = ?" for col in columns])
        query = f"UPDATE classes SET {set_clause}, updated_at = CURRENT_TIMESTAMP WHERE id = ?"
        values = [record_data[col] for col in columns] + [record_data['id']]
    try:
        with get_db_connection() as conn:
            cursor = conn.execute(query, values)
            conn.commit()
            if is_new:
                record_data['id'] = cursor.lastrowid
            return True
    except Exception as e:
        st.error(f"Error saving record: {e}")
        return False

def copy_class_records(class_ids):
    """Copy selected class records"""
    for class_id in class_ids:
        record = load_class_record(class_id)
        if record:
            record['name'] = record['name'] + " (Copy)"
            record.pop('id', None)
            save_class_record(record, is_new=True)

def delete_class_records(class_ids: list) -> Tuple[bool, str]:
    """Delete multiple class records by IDs"""
    if not class_ids:
        return False, "No records selected for deletion"
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")
        cursor.executemany("DELETE FROM classes WHERE id = ? AND is_racial = FALSE", [(id,) for id in class_ids])
        deleted_count = cursor.rowcount
        if deleted_count == len(class_ids):
            conn.commit()
            return True, f"Deleted {deleted_count} class(es) successfully"
        else:
            conn.rollback()
            return False, "Some classes could not be deleted"
    except Exception as e:
        conn.rollback()
        return False, f"Error deleting classes: {str(e)}"
    finally:
        conn.close()

def render_job_table():
    """Render the job classes table with simplified features"""
    st.header("Job Classes Table")

    # Initialize session state if not already set
    if 'page' not in st.session_state:
        st.session_state.page = 0

    # Handle query parameters to update session state
    query_params = st.query_params
    if 'page' in query_params:
        try:
            st.session_state.page = int(query_params['page'])
        except ValueError:
            st.session_state.page = 0

    # Fixed records per page
    records_per_page = 25

    # Load records for the current page
    offset = st.session_state.page * records_per_page
    df = load_job_classes(limit=records_per_page, offset=offset)

    # Render table if data exists
    if df.empty:
        st.warning("No job classes found. Click 'New Record' below to add one.")
    else:
        # Reverted to original working URL format
        editor_url = "http://localhost:8501/?script=job_class_editor&mode=edit"
        df['Edit'] = df['id'].apply(
            lambda x: f'<a href="{editor_url}&edit_id={x}" target="_blank">Edit</a>'
        )
        st.write(
            df[['id', 'name', 'class_type', 'category_id', 'subcategory_id', 'Edit']].to_html(escape=False, index=False),
            unsafe_allow_html=True
        )

    # New Record button under the table
    if st.button("New Record", key="new_record"):
        st.query_params.update({"script": "job_class_editor", "mode": "create"})
        st.rerun()

    # Pagination controls on a single row without dropdown
    st.write("")  # Spacer

    # Calculate pagination details
    total_records = get_total_job_classes()
    total_pages = (total_records // records_per_page) + (1 if total_records % records_per_page else 0)
    current_page = st.session_state.page
    if current_page >= total_pages:
        current_page = total_pages - 1 if total_pages > 0 else 0
        st.session_state.page = current_page
    elif current_page < 0:
        current_page = 0
        st.session_state.page = current_page

    prev_page = max(0, current_page - 1)
    next_page = min(total_pages - 1, current_page + 1)

    # "Previous" button
    if current_page > 0:
        prev_button = f'<button onclick="window.location.href=\'?page={prev_page}\'">Previous</button>'
    else:
        prev_button = '<button disabled>Previous</button>'

    # "Next" button
    if current_page < total_pages - 1:
        next_button = f'<button onclick="window.location.href=\'?page={next_page}\'">Next</button>'
    else:
        next_button = '<button disabled>Next</button>'

    # Page information
    page_info = f"Page {current_page + 1} of {total_pages}"

    # Combine pagination elements into HTML (no dropdown)
    pagination_html = f"""
    <div class="pagination-container">
        <div class="pagination-item">{prev_button}</div>
        <div class="pagination-item">{page_info}</div>
        <div class="pagination-item">{next_button}</div>
    </div>
    """

    # Define CSS for layout and styling
    st.markdown("""
        <style>
        .pagination-container {
            display: flex;
            flex-wrap: nowrap;
            justify-content: space-between;
            align-items: center;
            width: 100%;
            gap: 10px;
            padding: 10px 0;
        }
        .pagination-item {
            flex: 1;
            text-align: center;
            min-width: 0;
        }
        .pagination-container button {
            padding: 5px 10px;
            margin: 0 5px;
            border: 1px solid #ccc;
            background-color: #f0f0f0;
            color: #333;
            cursor: pointer;
        }
        .pagination-container button[disabled] {
            color: #999;
            cursor: not-allowed;
            background-color: #e0e0e0;
        }
        </style>
    """, unsafe_allow_html=True)

    # Render pagination
    st.markdown(pagination_html, unsafe_allow_html=True)

if __name__ == "__main__":
    render_job_table()