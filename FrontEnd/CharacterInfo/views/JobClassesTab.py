# ./FrontEnd/CharacterInfo/views/JobClassesTab.py

import streamlit as st
from typing import Dict, List, Optional, Tuple
from ..utils.database import get_db_connection

def get_class_types() -> List[Dict]:
    """Get all available class types"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_types 
            ORDER BY name
        """)
        results = cursor.fetchall()
        return [{"id": r[0], "name": r[1]} for r in results]
    finally:
        conn.close()

def get_class_categories() -> List[Dict]:
    """Get all class categories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name, is_racial 
            FROM class_categories 
            ORDER BY is_racial DESC, name
        """)
        results = cursor.fetchall()
        return [{"id": r[0], "name": r[1], "is_racial": r[2]} for r in results]
    finally:
        conn.close()

def get_class_subcategories() -> List[Dict]:
    """Get all class subcategories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_subcategories 
            ORDER BY name
        """)
        results = cursor.fetchall()
        return [{"id": r[0], "name": r[1]} for r in results]
    finally:
        conn.close()

def get_class_by_id(class_id: int) -> Optional[Dict]:
    """Get full class details by ID"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.*,
                ct.name as type_name,
                cc.name as category_name,
                cs.name as subcategory_name
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            WHERE c.id = ?
        """, (class_id,))
        result = cursor.fetchone()
        if not result:
            return None
        
        columns = [col[0] for col in cursor.description]
        return dict(zip(columns, result))
    finally:
        conn.close()

def save_class(class_data: Dict) -> Tuple[bool, str]:
    """Save or update a class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")
        
        if 'id' in class_data and class_data['id']:
            # Update existing class
            cursor.execute("""
                UPDATE classes SET
                    name = ?,
                    description = ?,
                    class_type = ?,
                    is_racial = ?,
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
                WHERE id = ?
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'], class_data['is_racial'],
                class_data['category_id'], class_data['subcategory_id'],
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
            message = "Class updated successfully"
        else:
            # Create new class
            cursor.execute("""
                INSERT INTO classes (
                    name, description, class_type, is_racial, category_id, subcategory_id,
                    base_hp, base_mp, base_physical_attack, base_physical_defense,
                    base_agility, base_magical_attack, base_magical_defense,
                    base_resistance, base_special,
                    hp_per_level, mp_per_level, physical_attack_per_level,
                    physical_defense_per_level, agility_per_level, magical_attack_per_level,
                    magical_defense_per_level, resistance_per_level, special_per_level
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                class_data['name'], class_data['description'],
                class_data['class_type'], class_data['is_racial'],
                class_data['category_id'], class_data['subcategory_id'],
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
            message = "Class created successfully"
        
        cursor.execute("COMMIT")
        return True, message
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error saving class: {str(e)}"
    finally:
        conn.close()

def render_class_editor(class_data: Optional[Dict] = None):
    """Render the class editor form"""
    # Load lookup data
    class_types = get_class_types()
    categories = get_class_categories()
    subcategories = get_class_subcategories()

    # Use different form keys for edit vs create
    form_key = f"class_editor_edit_{class_data['id']}" if class_data else "class_editor_create"
    
    with st.form(form_key):
        # Basic Information
        st.subheader("Basic Information")
        col1, col2 = st.columns(2)
        
        with col1:
            name = st.text_input(
                "Class Name",
                value=class_data.get('name', '') if class_data else ''
            )
            
            if class_data:
                default_index = next(
                    (i for i, t in enumerate(class_types) if t["id"] == class_data.get('class_type')), 
                    0
                )
            else:
                default_index = 0
                
            class_type = st.selectbox(
                "Class Type",
                options=[t["id"] for t in class_types],
                format_func=lambda x: next(t["name"] for t in class_types if t["id"] == x),
                index=default_index
            )
            
            if class_data:
                default_category_index = next(
                    (i for i, c in enumerate(categories) if c["id"] == class_data.get('category_id')), 
                    0
                )
            else:
                default_category_index = 0
                
            category = st.selectbox(
                "Category",
                options=[c["id"] for c in categories],
                format_func=lambda x: next(c["name"] for c in categories if c["id"] == x),
                index=default_category_index
            )
        
        with col2:
            is_racial = st.checkbox(
                "Is Racial Class",
                value=class_data.get('is_racial', False) if class_data else False
            )
            
            if class_data:
                default_subcategory_index = next(
                    (i for i, s in enumerate(subcategories) if s["id"] == class_data.get('subcategory_id')), 
                    0
                )
            else:
                default_subcategory_index = 0
                
            subcategory = st.selectbox(
                "Subcategory",
                options=[s["id"] for s in subcategories],
                format_func=lambda x: next(s["name"] for s in subcategories if s["id"] == x),
                index=default_subcategory_index
            )

        description = st.text_area(
            "Description",
            value=class_data.get('description', '') if class_data else ''
        )

        # Base Stats
        st.subheader("Base Stats")
        col1, col2, col3 = st.columns(3)
        
        with col1:
            base_hp = st.number_input("Base HP", value=class_data.get('base_hp', 0) if class_data else 0)
            base_mp = st.number_input("Base MP", value=class_data.get('base_mp', 0) if class_data else 0)
            base_physical_attack = st.number_input(
                "Base Physical Attack",
                value=class_data.get('base_physical_attack', 0) if class_data else 0
            )
            base_physical_defense = st.number_input(
                "Base Physical Defense",
                value=class_data.get('base_physical_defense', 0) if class_data else 0
            )
        
        with col2:
            base_agility = st.number_input(
                "Base Agility",
                value=class_data.get('base_agility', 0) if class_data else 0
            )
            base_magical_attack = st.number_input(
                "Base Magical Attack",
                value=class_data.get('base_magical_attack', 0) if class_data else 0
            )
            base_magical_defense = st.number_input(
                "Base Magical Defense",
                value=class_data.get('base_magical_defense', 0) if class_data else 0
            )
        
        with col3:
            base_resistance = st.number_input(
                "Base Resistance",
                value=class_data.get('base_resistance', 0) if class_data else 0
            )
            base_special = st.number_input(
                "Base Special",
                value=class_data.get('base_special', 0) if class_data else 0
            )

        # Per Level Gains
        st.subheader("Per Level Gains")
        col1, col2, col3 = st.columns(3)
        
        with col1:
            hp_per_level = st.number_input(
                "HP per Level",
                value=class_data.get('hp_per_level', 0) if class_data else 0
            )
            mp_per_level = st.number_input(
                "MP per Level",
                value=class_data.get('mp_per_level', 0) if class_data else 0
            )
            physical_attack_per_level = st.number_input(
                "Physical Attack per Level",
                value=class_data.get('physical_attack_per_level', 0) if class_data else 0
            )
        
        with col2:
            physical_defense_per_level = st.number_input(
                "Physical Defense per Level",
                value=class_data.get('physical_defense_per_level', 0) if class_data else 0
            )
            agility_per_level = st.number_input(
                "Agility per Level",
                value=class_data.get('agility_per_level', 0) if class_data else 0
            )
            magical_attack_per_level = st.number_input(
                "Magical Attack per Level",
                value=class_data.get('magical_attack_per_level', 0) if class_data else 0
            )
        
        with col3:
            magical_defense_per_level = st.number_input(
                "Magical Defense per Level",
                value=class_data.get('magical_defense_per_level', 0) if class_data else 0
            )
            resistance_per_level = st.number_input(
                "Resistance per Level",
                value=class_data.get('resistance_per_level', 0) if class_data else 0
            )
            special_per_level = st.number_input(
                "Special per Level",
                value=class_data.get('special_per_level', 0) if class_data else 0
            )

        # Submit button
        if st.form_submit_button("Save Class"):
            # Prepare class data
            new_class_data = {
                'id': class_data.get('id') if class_data else None,
                'name': name,
                'description': description,
                'class_type': class_type,
                'is_racial': is_racial,
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
            
            success, message = save_class(new_class_data)
            if success:
                st.success(message)
                return True
            else:
                st.error(message)

# ./FrontEnd/CharacterInfo/views/JobClassesTab.py (continued from previous implementation)

def get_all_classes() -> List[Dict]:
    """Get list of all classes"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.is_racial,
                ct.name as type_name,
                cc.name as category_name,
                cs.name as subcategory_name
            FROM classes c
            JOIN class_types ct ON c.class_type = ct.id
            JOIN class_categories cc ON c.category_id = cc.id
            LEFT JOIN class_subcategories cs ON c.subcategory_id = cs.id
            ORDER BY c.is_racial DESC, cc.name, c.name
        """)
        columns = [col[0] for col in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def delete_class(class_id: int) -> Tuple[bool, str]:
    """Delete a class"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        # First check if the class is in use
        cursor.execute("""
            SELECT COUNT(*) 
            FROM character_class_progression 
            WHERE class_id = ?
        """, (class_id,))
        if cursor.fetchone()[0] > 0:
            return False, "Cannot delete: Class is currently in use by one or more characters"

        cursor.execute("BEGIN TRANSACTION")
        
        # Delete prerequisites
        cursor.execute("DELETE FROM class_prerequisites WHERE class_id = ?", (class_id,))
        
        # Delete exclusions
        cursor.execute("DELETE FROM class_exclusions WHERE class_id = ?", (class_id,))
        
        # Delete the class
        cursor.execute("DELETE FROM classes WHERE id = ?", (class_id,))
        
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
    
    # Create tabs for different operations
    list_tab, create_tab = st.tabs(["Edit Existing Class", "Create New Class"])
    
    with list_tab:
        st.subheader("Select Class to Edit")
        
        # Get all classes
        classes = get_all_classes()
        
        # Group classes by category
        class_groups = {}
        for c in classes:
            category = "Racial Classes" if c['is_racial'] else "Job Classes"
            if category not in class_groups:
                class_groups[category] = []
            class_groups[category].append(c)
        
        # Create selection columns
        col1, col2 = st.columns([2, 1])
        
        with col1:
            # Create a selectbox for each category
            selected_class = None
            for category, group_classes in class_groups.items():
                st.markdown(f"**{category}**")
                class_options = [c['id'] for c in group_classes]
                class_labels = [
                    f"{c['name']} ({c['type_name']}" + 
                    (f" - {c['subcategory_name']}" if c['subcategory_name'] else "") + ")"
                    for c in group_classes
                ]
                
                selected_id = st.selectbox(
                    "Select Class",
                    options=class_options,
                    format_func=lambda x: next(
                        label for i, label in enumerate(class_labels)
                        if group_classes[i]['id'] == x
                    ),
                    key=f"select_{category}"
                )
                
                if selected_id:
                    selected_class = next(
                        c for c in group_classes if c['id'] == selected_id
                    )
        
        with col2:
            if selected_class:
                st.markdown("**Actions**")
                delete_btn = st.button(
                    "Delete Class",
                    help="Permanently delete this class",
                    key="delete_class"
                )
                
                if delete_btn:
                    if st.button("Confirm Delete", key="confirm_delete"):
                        success, message = delete_class(selected_class['id'])
                        if success:
                            st.success(message)
                            st.rerun()
                        else:
                            st.error(message)
        
        # Show editor if class is selected
        if selected_class:
            st.markdown("---")
            st.subheader(f"Editing: {selected_class['name']}")
            class_data = get_class_by_id(selected_class['id'])
            if class_data:
                if render_class_editor(class_data):
                    st.rerun()
            else:
                st.error("Error loading class data")
    
    with create_tab:
        st.subheader("Create New Class")
        if render_class_editor():
            st.rerun()