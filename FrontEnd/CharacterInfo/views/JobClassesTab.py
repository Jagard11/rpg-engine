# ./FrontEnd/CharacterInfo/views/JobClassesTab.py

import streamlit as st
import pandas as pd
from typing import Dict, List, Optional, Tuple
from ..utils.database import get_db_connection

DEFAULT_QUERY = """
SELECT 
    c.id,
    c.name,
    c.description,
    ct.name as type,
    cc.name as category,
    cs.name as subcategory
FROM classes c 
JOIN class_types ct ON c.class_type = ct.id
JOIN class_categories cc ON c.category_id = cc.id
LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
WHERE c.is_racial = 0
ORDER BY cc.name, c.name;
"""

def execute_query(query: str) -> Tuple[List[Dict], List[str]]:
    """Execute SQL query and return results with column names"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute(query)
        columns = [description[0] for description in cursor.description]
        results = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return results, columns
    finally:
        conn.close()

def get_class_types() -> List[Dict]:
    """Get all class types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM class_types ORDER BY name")
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_categories() -> List[Dict]:
    """Get non-racial class categories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_categories 
            WHERE is_racial = 0 
            ORDER BY name
        """)
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_subcategories() -> List[Dict]:
    """Get all class subcategories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM class_subcategories ORDER BY name")
        return [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_class_details(class_id: int) -> Optional[Dict]:
    """Get full details of a specific class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT * FROM classes WHERE id = ? AND is_racial = 0
        """, (class_id,))
        result = cursor.fetchone()
        if result:
            columns = [desc[0] for desc in cursor.description]
            return dict(zip(columns, result))
        return None
    finally:
        conn.close()

def save_class(class_data: Dict) -> Tuple[bool, str]:
    """Save or update a class record"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")
        
        if class_data.get('id'):
            # Update existing class
            cursor.execute("""
                UPDATE classes SET
                    name = ?,
                    description = ?,
                    class_type = ?,
                    category_id = ?,
                    subcategory_id = ?,
                    base_hp = ?,
                    base_mp = ?,
                    base_physical_attack = ?,
                    base_physical_defense = ?,
                    base_agility = ?,
                    base_magical_attack = ?,
                    base_magical_defense = ?,
                    base_resistance = ?,
                    base_special = ?,
                    hp_per_level = ?,
                    mp_per_level = ?,
                    physical_attack_per_level = ?,
                    physical_defense_per_level = ?,
                    agility_per_level = ?,
                    magical_attack_per_level = ?,
                    magical_defense_per_level = ?,
                    resistance_per_level = ?,
                    special_per_level = ?
                WHERE id = ? AND is_racial = 0
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'], class_data['category_id'],
                class_data['subcategory_id'],
                class_data['base_hp'], class_data['base_mp'],
                class_data['base_physical_attack'], class_data['base_physical_defense'],
                class_data['base_agility'], class_data['base_magical_attack'],
                class_data['base_magical_defense'], class_data['base_resistance'],
                class_data['base_special'], class_data['hp_per_level'],
                class_data['mp_per_level'], class_data['physical_attack_per_level'],
                class_data['physical_defense_per_level'], class_data['agility_per_level'],
                class_data['magical_attack_per_level'], class_data['magical_defense_per_level'],
                class_data['resistance_per_level'], class_data['special_per_level'],
                class_data['id']
            ))
        else:
            # Create new class
            cursor.execute("""
                INSERT INTO classes (
                    name, description, class_type, is_racial,
                    category_id, subcategory_id,
                    base_hp, base_mp, base_physical_attack, base_physical_defense,
                    base_agility, base_magical_attack, base_magical_defense,
                    base_resistance, base_special,
                    hp_per_level, mp_per_level, physical_attack_per_level,
                    physical_defense_per_level, agility_per_level,
                    magical_attack_per_level, magical_defense_per_level,
                    resistance_per_level, special_per_level
                ) VALUES (?, ?, ?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'], class_data['category_id'],
                class_data['subcategory_id'],
                class_data['base_hp'], class_data['base_mp'],
                class_data['base_physical_attack'], class_data['base_physical_defense'],
                class_data['base_agility'], class_data['base_magical_attack'],
                class_data['base_magical_defense'], class_data['base_resistance'],
                class_data['base_special'], class_data['hp_per_level'],
                class_data['mp_per_level'], class_data['physical_attack_per_level'],
                class_data['physical_defense_per_level'], class_data['agility_per_level'],
                class_data['magical_attack_per_level'], class_data['magical_defense_per_level'],
                class_data['resistance_per_level'], class_data['special_per_level']
            ))
        
        cursor.execute("COMMIT")
        return True, "Class saved successfully!"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error saving class: {str(e)}"
    finally:
        conn.close()

def delete_class(class_id: int) -> Tuple[bool, str]:
    """Delete a job class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # Check if class is in use
        cursor.execute("""
            SELECT COUNT(*) 
            FROM character_class_progression 
            WHERE class_id = ?
        """, (class_id,))
        if cursor.fetchone()[0] > 0:
            return False, "Cannot delete: Class is currently in use by one or more characters"

        # Check if it's a racial class
        cursor.execute("SELECT is_racial FROM classes WHERE id = ?", (class_id,))
        result = cursor.fetchone()
        if not result or result[0]:
            return False, "Cannot delete: Invalid class ID or racial class"

        cursor.execute("BEGIN TRANSACTION")
        
        # Delete prerequisites
        cursor.execute("DELETE FROM class_prerequisites WHERE class_id = ?", (class_id,))
        
        # Delete exclusions
        cursor.execute("DELETE FROM class_exclusions WHERE class_id = ?", (class_id,))
        
        # Delete class
        cursor.execute("DELETE FROM classes WHERE id = ? AND is_racial = 0", (class_id,))
        
        cursor.execute("COMMIT")
        return True, "Class deleted successfully"
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error deleting class: {str(e)}"
    finally:
        conn.close()

def render_job_classes_tab():
    """Main render function for the job classes tab"""
    st.header("Job Classes Editor")
    
    # Initialize session state for SQL query
    if 'sql_query' not in st.session_state:
        st.session_state.sql_query = DEFAULT_QUERY
    if 'selected_class_id' not in st.session_state:
        st.session_state.selected_class_id = None
    
    # SQL Query input
    st.session_state.sql_query = st.text_area(
        "SQL Query", 
        value=st.session_state.sql_query,
        height=100
    )
    
    try:
        # Execute query and display results
        results, columns = execute_query(st.session_state.sql_query)
        
        # Create DataFrame for display
        df_data = []
        for idx, row in enumerate(results):
            row_data = {'select': False}  # Initialize selection column
            for col in columns:
                val = row.get(col, '')
                # Truncate long text for better display
                if isinstance(val, str) and len(val) > 50:
                    val = val[:47] + "..."
                row_data[col] = val
            df_data.append(row_data)

        # Create DataFrame
        df = pd.DataFrame(df_data)

        # Display table with selection
        selection = st.data_editor(
            df,
            column_config={
                "select": st.column_config.CheckboxColumn(
                    "Select",
                    default=False,
                    help="Select a class to edit"
                )
            },
            disabled=[col for col in df.columns if col != 'select'],
            hide_index=True
        )

        # Handle selection
        if selection is not None and 'select' in selection:
            selected_rows = selection[selection['select']].index
            if len(selected_rows) > 0:
                selected_idx = selected_rows[0]
                st.session_state.selected_class_id = results[selected_idx]['id']
                # Deselect all other rows
                for idx in selected_rows[1:]:
                    selection.at[idx, 'select'] = False
            else:
                st.session_state.selected_class_id = None
            
        # Add/Remove buttons
        col1, col2 = st.columns(2)
        with col1:
            if st.button("Add New Class", use_container_width=True):
                st.session_state.selected_class_id = None
                st.rerun()
        with col2:
            if st.button("Delete Selected Class", use_container_width=True):
                if st.session_state.selected_class_id:
                    success, message = delete_class(st.session_state.selected_class_id)
                    if success:
                        st.success(message)
                        st.session_state.selected_class_id = None
                        st.rerun()
                    else:
                        st.error(message)
        
        # Display class details form if a class is selected or we're adding new
        st.markdown("---")
        if st.session_state.selected_class_id is not None or st.session_state.selected_class_id == None:
            class_data = get_class_details(st.session_state.selected_class_id) if st.session_state.selected_class_id else {}
            
            # Load lookup data
            class_types = get_class_types()
            categories = get_class_categories()
            subcategories = get_class_subcategories()
            
            # Create form
            with st.form("class_details_form"):
                st.subheader("Class Details")
                
                # Basic info
                col1, col2, col3 = st.columns(3)
                with col1:
                    name = st.text_input(
                        "Name",
                        value=class_data.get('name', '')
                    )
                with col2:
                    class_type = st.selectbox(
                        "Type",
                        options=[t["id"] for t in class_types],
                        format_func=lambda x: next(t["name"] for t in class_types if t["id"] == x),
                        index=next(
                            (i for i, t in enumerate(class_types) 
                             if t["id"] == class_data.get('class_type', 1)),
                            0
                        )
                    )
                with col3:
                    category = st.selectbox(
                        "Category",
                        options=[c["id"] for c in categories],
                        format_func=lambda x: next(c["name"] for c in categories if c["id"] == x),
                        index=next(
                            (i for i, c in enumerate(categories) 
                             if c["id"] == class_data.get('category_id', 1)),
                            0
                        )
                    )
                
                subcategory = st.selectbox(
                    "Subcategory",
                    options=[s["id"] for s in subcategories],
                    format_func=lambda x: next(s["name"] for s in subcategories if s["id"] == x),
                    index=next(
                        (i for i, s in enumerate(subcategories) 
                         if s["id"] == class_data.get('subcategory_id', 1)),
                        0
                    )
                )
                
                description = st.text_area(
                    "Description",
                    value=class_data.get('description', '')
                )
                
                # Stats
                col1, col2, col3 = st.columns(3)
                
                with col1:
                    st.markdown("**Base Stats**")
                    base_hp = st.number_input("Base HP", value=class_data.get('base_hp', 0))
                    base_mp = st.number_input("Base MP", value=class_data.get('base_mp', 0))
                    base_physical_attack = st.number_input("Base Physical Attack", value=class_data.get('base_physical_attack', 0))
                    base_physical_defense = st.number_input("Base Physical Defense", value=class_data.get('base_physical_defense', 0))
                    base_agility = st.number_input("Base Agility", value=class_data.get('base_agility', 0))
                    base_magical_attack = st.number_input("Base Magical Attack", value=class_data.get('base_magical_attack', 0))
                    base_magical_defense = st.number_input("Base Magical Defense", value=class_data.get('base_magical_defense', 0))
                    base_resistance = st.number_input("Base Resistance", value=class_data.get('base_resistance', 0))
                    base_special = st.number_input("Base Special", value=class_data.get('base_special', 0))

                with col2:
                    st.markdown("**Per Level Stats (1/2)**")
                    hp_per_level = st.number_input("HP per Level", value=class_data.get('hp_per_level', 0))
                    mp_per_level = st.number_input("MP per Level", value=class_data.get('mp_per_level', 0))
                    physical_attack_per_level = st.number_input("Physical Attack per Level", value=class_data.get('physical_attack_per_level', 0))
                    physical_defense_per_level = st.number_input("Physical Defense per Level", value=class_data.get('physical_defense_per_level', 0))
                    agility_per_level = st.number_input("Agility per Level", value=class_data.get('agility_per_level', 0))

                with col3:
                    st.markdown("**Per Level Stats (2/2)**")
                    magical_attack_per_level = st.number_input("Magical Attack per Level", value=class_data.get('magical_attack_per_level', 0))
                    magical_defense_per_level = st.number_input("Magical Defense per Level", value=class_data.get('magical_defense_per_level', 0))
                    resistance_per_level = st.number_input("Resistance per Level", value=class_data.get('resistance_per_level', 0))
                    special_per_level = st.number_input("Special per Level", value=class_data.get('special_per_level', 0))

                # Save button
                if st.form_submit_button("Save Changes", use_container_width=True):
                    # Prepare class data
                    updated_class_data = {
                        'id': st.session_state.selected_class_id,
                        'name': name,
                        'description': description,
                        'class_type': class_type,
                        'category_id': category,
                        'subcategory_id': subcategory,
                        'base_hp': base_hp,
                        'base_mp': base_mp,
                        'base_physical_attack': base_physical_attack,
                        'base_physical_defense': base_physical_defense,
                        'base_agility': base_agility,
                        'base_magical_attack': base_magical_attack,
                        'base_magical_defense': base_magical_defense,
                        'base_resistance': base_resistance,
                        'base_special': base_special,
                        'hp_per_level': hp_per_level,
                        'mp_per_level': mp_per_level,
                        'physical_attack_per_level': physical_attack_per_level,
                        'physical_defense_per_level': physical_defense_per_level,
                        'agility_per_level': agility_per_level,
                        'magical_attack_per_level': magical_attack_per_level,
                        'magical_defense_per_level': magical_defense_per_level,
                        'resistance_per_level': resistance_per_level,
                        'special_per_level': special_per_level
                    }
                    
                    success, message = save_class(updated_class_data)
                    if success:
                        st.success(message)
                        st.rerun()
                    else:
                        st.error(message)
                        
    except Exception as e:
        st.error(f"Error executing query: {str(e)}")