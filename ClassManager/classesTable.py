# ./ClassManager/classesTable.py

import streamlit as st
import sqlite3
import pandas as pd
from pathlib import Path

def get_db_connection():
    """Create a database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def load_job_classes(limit=100, offset=0):
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

def render_job_table():
    """Render the job classes table with enhanced features"""
    st.header("Job Classes Table")

    # Pagination settings
    if 'page' not in st.session_state:
        st.session_state.page = 0
    if 'records_per_page' not in st.session_state:
        st.session_state.records_per_page = 50

    # Load records for the current page
    offset = st.session_state.page * st.session_state.records_per_page
    df = load_job_classes(limit=st.session_state.records_per_page, offset=offset)
    if df.empty:
        st.warning("No job classes found.")
        return

    # Add clickable Edit hyperlinks column with correct URL
    editor_url = "http://localhost:8501/?script=job_classeditor&mode=edit"
    df['Edit'] = df['id'].apply(
        lambda x: f'<a href="{editor_url}&edit_id={x}" target="_blank">Edit</a>'
    )

    # Add a selection checkbox column
    if 'selected_ids' not in st.session_state:
        st.session_state.selected_ids = []
    df['Select'] = df['id'].apply(
        lambda x: f'<input type="checkbox" name="select_{x}" {"checked" if x in st.session_state.selected_ids else ""}>'
    )

    # Display the table with hyperlinks and checkboxes using st.markdown
    st.subheader("Job Classes Table")
    st.write(
        df[['Select', 'id', 'name', 'class_type', 'category_id', 'subcategory_id', 'Edit']].to_html(escape=False, index=False),
        unsafe_allow_html=True
    )

    # Update selected_ids based on form submission
    with st.form(key="selection_form"):
        submit_button = st.form_submit_button(label="Update Selection")
        if submit_button:
            selected_ids = [
                int(k.split('_')[1]) for k, v in st.session_state.items()
                if k.startswith("select_") and v
            ]
            st.session_state.selected_ids = selected_ids

    # Action Buttons
    st.subheader("Actions")
    col1, col2, col3 = st.columns(3)
    with col1:
        if st.button("New Record", key="new_record"):
            st.query_params.update({"script": "job_classeditor", "mode": "create"})
            st.rerun()
    with col2:
        if st.session_state.selected_ids:
            if st.button("Edit Selected", key="edit_selected"):
                for _id in st.session_state.selected_ids:
                    url = f"{editor_url}&edit_id={_id}"
                    st.write(f'<script>window.open("{url}", "_blank")</script>', unsafe_allow_html=True)
        else:
            st.write("Select records to edit")
    with col3:
        if st.session_state.selected_ids:
            if st.button("Copy Selected", key="copy_selected"):
                copy_class_records(st.session_state.selected_ids)
                st.success(f"Copied {len(st.session_state.selected_ids)} record(s)")
                st.rerun()
        else:
            st.write("Select records to copy")

    # Pagination Controls with Records per Page Dropdown
    st.subheader("Pagination")
    col1, col2, col3, col4 = st.columns(4)
    with col1:
        if st.session_state.page > 0:
            if st.button("Previous", key="previous"):
                st.session_state.page -= 1
                st.rerun()
        else:
            st.write("First page")
    with col2:
        total_records = get_total_job_classes()
        total_pages = (total_records // st.session_state.records_per_page) + (1 if total_records % st.session_state.records_per_page else 0)
        st.write(f"Page {st.session_state.page + 1} of {total_pages}")
    with col3:
        records_per_page = st.selectbox(
            "Records per page",
            [25, 50, 100],
            index=[25, 50, 100].index(st.session_state.records_per_page),
            key="records_per_page_select"
        )
        if records_per_page != st.session_state.records_per_page:
            st.session_state.records_per_page = records_per_page
            st.session_state.page = 0  # Reset to first page on change
            st.rerun()
    with col4:
        if st.session_state.page < total_pages - 1:
            if st.button("Next", key="next"):
                st.session_state.page += 1
                st.rerun()
        else:
            st.write("Last page")

# Call the function to render the table
render_job_table()