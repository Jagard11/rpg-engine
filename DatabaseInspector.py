# ./DatabaseInspector.py

import streamlit as st
import sqlite3
import pandas as pd
from pathlib import Path
import json
from typing import List, Tuple, Dict, Optional

def get_tables() -> List[str]:
    """Get list of all tables in the database"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
    tables = [row[0] for row in cursor.fetchall()]
    conn.close()
    return tables

def get_foreign_key_info(table_name: str) -> List[Dict]:
    """Get foreign key information for a table"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    cursor.execute(f"PRAGMA foreign_key_list({table_name});")
    fk_info = cursor.fetchall()
    conn.close()
    
    foreign_keys = []
    for fk in fk_info:
        foreign_keys.append({
            'id': fk[0],              # id
            'seq': fk[1],             # seq
            'table': fk[2],           # referenced table
            'from': fk[3],            # column in current table
            'to': fk[4],              # column in referenced table
            'on_update': fk[5],       # on update
            'on_delete': fk[6],       # on delete
            'match': fk[7]            # match
        })
    return foreign_keys

def get_referenced_values(table_name: str, id_column: str, display_column: str) -> List[Tuple]:
    """Get values from a referenced table for dropdown"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    try:
        cursor.execute(f"SELECT {id_column}, {display_column} FROM {table_name}")
        return cursor.fetchall()
    except Exception as e:
        st.error(f"Error fetching referenced values: {str(e)}")
        return []
    finally:
        conn.close()

def get_primary_key_column(table_name: str) -> Optional[str]:
    """Get the primary key column name for a table"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    cursor.execute(f"PRAGMA table_info({table_name});")
    columns = cursor.fetchall()
    conn.close()
    
    for col in columns:
        if col[5] == 1:  # Primary key flag
            return col[1]
    return None

def get_display_column(table_name: str) -> str:
    """Get the best column to display for a table"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    cursor.execute(f"PRAGMA table_info({table_name});")
    columns = cursor.fetchall()
    conn.close()
    
    # Prefer 'name' column if it exists
    for col in columns:
        if col[1].lower() == 'name':
            return col[1]
    
    # Otherwise, return first non-primary-key column
    for col in columns:
        if col[5] != 1:  # Not a primary key
            return col[1]
    
    # Fallback to primary key if nothing else available
    return columns[0][1]

def get_table_schema(table_name: str) -> List[Tuple]:
    """Get schema information for a specific table"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    cursor.execute(f"PRAGMA table_info({table_name});")
    schema = cursor.fetchall()
    conn.close()
    return schema

def get_table_contents(table_name: str) -> pd.DataFrame:
    """Get all records from a specific table"""
    conn = sqlite3.connect('rpg_data.db')
    query = f"SELECT * FROM {table_name};"
    df = pd.read_sql_query(query, conn)
    conn.close()
    return df

def add_foreign_key(table_name: str, column_name: str, reference_table: str, reference_column: str):
    """Add foreign key constraint to existing column"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    try:
        # Create new table with foreign key
        cursor.execute(f"""
        CREATE TABLE new_{table_name} AS SELECT * FROM {table_name};
        """)
        
        # Get existing table schema
        cursor.execute(f"PRAGMA table_info({table_name});")
        columns = cursor.fetchall()
        
        # Construct column definitions
        column_defs = []
        for col in columns:
            name = col[1]
            type_ = col[2]
            not_null = "NOT NULL" if col[3] else ""
            pk = "PRIMARY KEY" if col[5] else ""
            
            if name == column_name:
                column_defs.append(
                    f"{name} {type_} {not_null} {pk} REFERENCES {reference_table}({reference_column})"
                )
            else:
                column_defs.append(f"{name} {type_} {not_null} {pk}")
        
        # Create new table with foreign key
        cursor.execute(f"""
        CREATE TABLE new_{table_name}_with_fk (
            {', '.join(column_defs)}
        );
        """)
        
        # Copy data
        cursor.execute(f"INSERT INTO new_{table_name}_with_fk SELECT * FROM {table_name};")
        
        # Drop old table and rename new one
        cursor.execute(f"DROP TABLE {table_name};")
        cursor.execute(f"ALTER TABLE new_{table_name}_with_fk RENAME TO {table_name};")
        
        conn.commit()
        return True, "Foreign key added successfully"
    except Exception as e:
        return False, str(e)
    finally:
        conn.close()

def insert_record(table_name: str, data: dict):
    """Insert a new record into the table"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    columns = ', '.join(data.keys())
    placeholders = ', '.join(['?' for _ in data])
    query = f"INSERT INTO {table_name} ({columns}) VALUES ({placeholders})"
    
    try:
        cursor.execute(query, list(data.values()))
        conn.commit()
        return True, "Record inserted successfully"
    except Exception as e:
        return False, str(e)
    finally:
        conn.close()

def update_record(table_name: str, record_id: int, data: dict):
    """Update an existing record in the table"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    set_clause = ', '.join([f"{k} = ?" for k in data.keys()])
    query = f"UPDATE {table_name} SET {set_clause} WHERE id = ?"
    
    try:
        cursor.execute(query, list(data.values()) + [record_id])
        conn.commit()
        return True, "Record updated successfully"
    except Exception as e:
        return False, str(e)
    finally:
        conn.close()

def delete_record(table_name: str, record_id: int):
    """Delete a record from the table"""
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    try:
        cursor.execute(f"DELETE FROM {table_name} WHERE id = ?", (record_id,))
        conn.commit()
        return True, "Record deleted successfully"
    except Exception as e:
        return False, str(e)
    finally:
        conn.close()

def render_add_record_form(table_name: str, schema_df: pd.DataFrame):
    """Render the form for adding a new record with foreign key support"""
    st.subheader("Add New Record")
    
    # Get foreign key information
    foreign_keys = get_foreign_key_info(table_name)
    fk_dict = {fk['from']: fk for fk in foreign_keys}
    
    new_record = {}
    for _, row in schema_df.iterrows():
        col_name = row['Column Name']
        if col_name != 'id':  # Skip ID field for new records
            if col_name in fk_dict:
                # Handle foreign key field
                fk = fk_dict[col_name]
                referenced_table = fk['table']
                referenced_pk = get_primary_key_column(referenced_table)
                display_column = get_display_column(referenced_table)
                
                # Get values for dropdown
                options = get_referenced_values(referenced_table, referenced_pk, display_column)
                
                # Create selectbox with formatted options
                selected = st.selectbox(
                    f"{col_name} ({referenced_table})",
                    options=options,
                    format_func=lambda x: f"{x[1]} (ID: {x[0]})"
                )
                if selected:
                    new_record[col_name] = selected[0]
            
            elif 'json' in row['Data Type'].lower():
                # Special handling for JSON fields
                json_input = st.text_area(f"{col_name} (JSON)", "{}")
                try:
                    new_record[col_name] = json.loads(json_input)
                except:
                    st.error(f"Invalid JSON for {col_name}")
            else:
                new_record[col_name] = st.text_input(col_name)
    
    if st.button("Add Record"):
        success, message = insert_record(table_name, new_record)
        if success:
            st.success(message)
        else:
            st.error(message)

def render_db_inspector_tab():
    """Render the database inspector interface"""
    st.header("Database Inspector")

    # Tabs for different operations
    inspector_tab, table_management_tab = st.tabs(["Data Inspector", "Table Management"])
    
    with inspector_tab:
        render_data_inspector()
        
    with table_management_tab:
        render_table_management()

def render_data_inspector():
    """Render the data inspection and manipulation interface"""
    # Get list of tables
    tables = get_tables()
    
    # Table selection
    selected_table = st.selectbox("Select Table", tables, key="inspector_table_select")
    
    if selected_table:
        # Show table schema
        st.subheader(f"Schema for {selected_table}")
        schema = get_table_schema(selected_table)
        schema_df = pd.DataFrame(
            schema, 
            columns=['cid', 'name', 'type', 'notnull', 'dflt_value', 'pk']
        )[['name', 'type', 'notnull', 'pk']]
        schema_df.columns = ['Column Name', 'Data Type', 'Not Null', 'Primary Key']
        st.dataframe(schema_df)
        
        # CRUD Operations
        crud_tab1, crud_tab2, crud_tab3 = st.tabs(["View/Edit Data", "Add Record", "Delete Record"])
        
        with crud_tab1:
            # Show editable table contents
            st.subheader(f"Contents of {selected_table}")
            contents = get_table_contents(selected_table)
            
            # Display with editing capability
            edited_df = st.data_editor(contents, num_rows="dynamic")
            
            # Save changes button
            if st.button("Save Changes"):
                try:
                    for index, row in edited_df.iterrows():
                        if row['id'] in contents['id'].values:
                            # Update existing record
                            success, message = update_record(
                                selected_table,
                                row['id'],
                                row.to_dict()
                            )
                        else:
                            # Insert new record
                            success, message = insert_record(
                                selected_table,
                                row.to_dict()
                            )
                        if not success:
                            st.error(f"Error: {message}")
                    st.success("Changes saved successfully!")
                except Exception as e:
                    st.error(f"Error saving changes: {str(e)}")
        
        with crud_tab2:
            render_add_record_form(selected_table, schema_df)
        
        with crud_tab3:
            st.subheader("Delete Record")
            contents = get_table_contents(selected_table)
            record_to_delete = st.selectbox(
                "Select Record to Delete",
                contents.apply(lambda x: f"ID: {x['id']} - {x.to_dict()}", axis=1)
            )
            
            if st.button("Delete Selected Record"):
                record_id = int(record_to_delete.split()[1])
                success, message = delete_record(selected_table, record_id)
                if success:
                    st.success(message)
                else:
                    st.error(message)

def modify_table_schema(table_name: str, operations: List[dict]):
    """Modify an existing table's schema
    operations: List of operations to perform, each containing:
        - action: 'add', 'remove', or 'modify'
        - column: column name
        - details: new column details (for add/modify)
    """
    conn = sqlite3.connect('rpg_data.db')
    cursor = conn.cursor()
    
    try:
        # Get current schema
        cursor.execute(f"PRAGMA table_info({table_name});")
        current_schema = cursor.fetchall()
        
        # Create new table name
        temp_table_name = f"temp_{table_name}"
        
        # Build new column definitions
        new_columns = []
        old_columns = []  # for copying data
        
        # Process existing columns and modifications
        for col in current_schema:
            col_name = col[1]
            col_type = col[2]
            not_null = "NOT NULL" if col[3] else ""
            pk = "PRIMARY KEY" if col[5] else ""
            
            # Check if this column should be removed or modified
            should_remove = any(op['action'] == 'remove' and op['column'] == col_name 
                              for op in operations)
            modify_op = next((op for op in operations 
                            if op['action'] == 'modify' and op['column'] == col_name), None)
            
            if not should_remove:
                if modify_op:
                    # Add modified column
                    details = modify_op['details']
                    new_columns.append(
                        f"{col_name} {details['type']} "
                        f"{'NOT NULL' if details.get('not_null') else ''} "
                        f"{'PRIMARY KEY' if details.get('primary_key') else ''}"
                    )
                else:
                    # Keep existing column
                    new_columns.append(f"{col_name} {col_type} {not_null} {pk}")
                old_columns.append(col_name)
        
        # Add new columns
        for op in operations:
            if op['action'] == 'add':
                details = op['details']
                new_columns.append(
                    f"{op['column']} {details['type']} "
                    f"{'NOT NULL' if details.get('not_null') else ''} "
                    f"{'PRIMARY KEY' if details.get('primary_key') else ''}"
                )
                # New columns won't be in old_columns as they don't exist in original table
        
        # Create new table with modified schema
        create_query = f"""
        CREATE TABLE {temp_table_name} (
            {', '.join(new_columns)}
        );
        """
        cursor.execute(create_query)
        
        # Copy data from old table to new table
        if old_columns:
            old_cols_str = ', '.join(old_columns)
            cursor.execute(f"INSERT INTO {temp_table_name}({old_cols_str}) SELECT {old_cols_str} FROM {table_name};")
        
        # Drop old table and rename new table
        cursor.execute(f"DROP TABLE {table_name};")
        cursor.execute(f"ALTER TABLE {temp_table_name} RENAME TO {table_name};")
        
        conn.commit()
        return True, "Table schema modified successfully"
    except Exception as e:
        return False, str(e)
    finally:
        conn.close()

def render_table_management():
    """Render the table management interface"""
    st.subheader("Table Schema Management")
    
    # Get list of tables
    tables = get_tables()
    
    management_type = st.radio(
        "Select Operation",
        ["Create New Table", "Modify Existing Table"],
        key="table_management_type"
    )
    
    if management_type == "Create New Table":
        # Table name input
        new_table_name = st.text_input("New Table Name")
        
        # Dynamic column definition
        st.write("Define Columns:")
        num_columns = st.number_input("Number of Columns", min_value=1, value=1)
        
        columns = []
        for i in range(int(num_columns)):
            st.markdown(f"#### Column {i+1}")
            col1, col2, col3, col4 = st.columns(4)
            with col1:
                col_name = st.text_input(f"Column Name", key=f"col_name_{i}")
            with col2:
                col_type = st.selectbox(
                    "Data Type",
                    ["INTEGER", "TEXT", "REAL", "BLOB", "JSON"],
                    key=f"col_type_{i}"
                )
            with col3:
                col_pk = st.checkbox("Primary Key", key=f"pk_{i}")
            with col4:
                col_not_null = st.checkbox("Not Null", key=f"nn_{i}")
            
            if col_name:
                columns.append({
                    'name': col_name,
                    'type': col_type,
                    'primary_key': col_pk,
                    'not_null': col_not_null
                })
        
        # Create table button
        if st.button("Create New Table"):
            if new_table_name and columns:
                # Construct CREATE TABLE query
                column_defs = []
                for col in columns:
                    definition = f"{col['name']} {col['type']}"
                    if col.get('primary_key'):
                        definition += " PRIMARY KEY"
                    if col.get('not_null'):
                        definition += " NOT NULL"
                column_defs.append(definition)
                
                query = f"CREATE TABLE {new_table_name} (\n" + ",\n".join(column_defs) + "\n);"
                
                # Execute the query
                conn = sqlite3.connect('rpg_data.db')
                cursor = conn.cursor()
                try:
                    cursor.execute(query)
                    conn.commit()
                    st.success("Table created successfully!")
                except Exception as e:
                    st.error(f"Error creating table: {str(e)}")
                finally:
                    conn.close()
            else:
                st.warning("Please provide a table name and at least one column")
    
    else:  # Modify Existing Table
        selected_table = st.selectbox("Select Table to Modify", tables)
        
        if selected_table:
            # Show current schema
            st.subheader("Current Schema")
            schema = get_table_schema(selected_table)
            schema_df = pd.DataFrame(
                schema,
                columns=['cid', 'name', 'type', 'notnull', 'dflt_value', 'pk']
            )[['name', 'type', 'notnull', 'pk']]
            schema_df.columns = ['Column Name', 'Data Type', 'Not Null', 'Primary Key']
            st.dataframe(schema_df)
            
            # Schema modification options
            st.subheader("Modify Schema")
            modification_type = st.selectbox(
                "Select Modification Type",
                ["Add Column", "Remove Column", "Modify Column", "Add Foreign Key"]
            )
            
            operations = []
            
            if modification_type == "Add Column":
                col1, col2, col3, col4 = st.columns(4)
                with col1:
                    new_col_name = st.text_input("New Column Name")
                with col2:
                    new_col_type = st.selectbox(
                        "Data Type",
                        ["INTEGER", "TEXT", "REAL", "BLOB", "JSON"]
                    )
                with col3:
                    new_col_pk = st.checkbox("Primary Key")
                with col4:
                    new_col_not_null = st.checkbox("Not Null")
                
                if new_col_name:
                    operations.append({
                        'action': 'add',
                        'column': new_col_name,
                        'details': {
                            'type': new_col_type,
                            'primary_key': new_col_pk,
                            'not_null': new_col_not_null
                        }
                    })
            
            elif modification_type == "Remove Column":
                col_to_remove = st.selectbox(
                    "Select Column to Remove",
                    [row['Column Name'] for _, row in schema_df.iterrows()]
                )
                if col_to_remove:
                    operations.append({
                        'action': 'remove',
                        'column': col_to_remove
                    })
            
            elif modification_type == "Add Foreign Key":
                # Select column to add foreign key to
                col_to_modify = st.selectbox(
                    "Select Column",
                    [row['Column Name'] for _, row in schema_df.iterrows() if row['Data Type'] == 'INTEGER']
                )
                
                # Select reference table
                reference_table = st.selectbox(
                    "Reference Table",
                    [table for table in tables if table != selected_table]
                )
                
                if reference_table:
                    if st.button("Add Foreign Key"):
                        success, message = add_foreign_key(
                            selected_table,
                            col_to_modify,
                            reference_table,
                            "id"  # Hardcode to reference the id column
                        )
                        if success:
                            st.success(message)
                        else:
                            st.error(message)
            
            elif modification_type == "Modify Column":
                col_to_modify = st.selectbox(
                    "Select Column to Modify",
                    [row['Column Name'] for _, row in schema_df.iterrows()]
                )
                
                if col_to_modify:
                    col1, col2, col3, col4 = st.columns(4)
                    with col1:
                        st.write(f"Column: {col_to_modify}")
                    with col2:
                        mod_col_type = st.selectbox(
                            "New Data Type",
                            ["INTEGER", "TEXT", "REAL", "BLOB", "JSON"]
                        )
                    with col3:
                        mod_col_pk = st.checkbox("Primary Key")
                    with col4:
                        mod_col_not_null = st.checkbox("Not Null")
                    
                    operations.append({
                        'action': 'modify',
                        'column': col_to_modify,
                        'details': {
                            'type': mod_col_type,
                            'primary_key': mod_col_pk,
                            'not_null': mod_col_not_null
                        }
                    })
            
            if st.button("Apply Schema Changes"):
                if operations:
                    success, message = modify_table_schema(selected_table, operations)
                    if success:
                        st.success(message)
                    else:
                        st.error(message)
                else:
                    st.warning("No changes specified")