# ./ClassManager/classesTable.py

import streamlit as st
import sqlite3
import pandas as pd
from pathlib import Path

def get_db_connection():
    """Create a database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def execute_sql_query(query: str) -> pd.DataFrame:
    """Execute SQL query and return results as DataFrame"""
    try:
        with get_db_connection() as conn:
            df = pd.read_sql_query(query, conn)
            return df
    except sqlite3.Error as e:
        st.error(f"Database error: {e}")
        return pd.DataFrame()
    except Exception as e:
        st.error(f"Error executing query: {e}")
        return pd.DataFrame()

def get_all_classes() -> pd.DataFrame:
    """Get all classes from database"""
    query = """
    SELECT c.*, 
           ct.name as class_type_name,
           cc.name as category_name,
           cs.name as subcategory_name
    FROM classes c
    LEFT JOIN class_types ct ON c.class_type = ct.id
    LEFT JOIN class_categories cc ON c.category_id = cc.id
    LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
    """
    return execute_sql_query(query)

def render_classes_table():
    """Renders the class list view with SQL search and selection"""
    st.header("Class List")
    
    # SQL Query Input
    default_query = """
    SELECT c.*, 
           ct.name as class_type_name,
           cc.name as category_name,
           cs.name as subcategory_name
    FROM classes c
    LEFT JOIN class_types ct ON c.class_type = ct.id
    LEFT JOIN class_categories cc ON c.category_id = cc.id
    LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
    """
    
    sql_query = st.text_area(
        "SQL Query",
        value=default_query,
        height=150,
        key="class_sql_query"
    )
    
    # Execute Query Button
    if st.button("Execute Query", key="execute_class_query"):
        df = execute_sql_query(sql_query)
        if not df.empty:
            st.session_state.class_query_results = df
            
    # Selection Dropdown
    if 'class_query_results' in st.session_state and not st.session_state.class_query_results.empty:
        class_names = st.session_state.class_query_results['name'].tolist()
        selected_class = st.selectbox(
            "Select Class to Edit",
            options=class_names,
            key="selected_class_name"
        )
        
        # Display results in a table
        st.subheader("Query Results")
        st.dataframe(
            st.session_state.class_query_results,
            use_container_width=True,
            hide_index=True
        )