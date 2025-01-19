# ./FrontEnd/CharacterInfo/views/RaceCreation.py

import streamlit as st
from typing import List, Tuple, Dict, Optional
from ..utils.database import get_db_connection

def get_available_race_categories() -> List[Tuple[int, str]]:
    """Get list of available race categories from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT id, name 
            FROM class_categories 
            WHERE is_racial = TRUE 
            ORDER BY name
        """)
        return cursor.fetchall()
    finally:
        conn.close()

def get_all_classes() -> List[Dict]:
    """Get all classes for prerequisite selection"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                c.id,
                c.name,
                c.is_racial,
                cat.name as category,
                sub.name as subcategory
            FROM classes c
            JOIN class_categories cat ON c.category_id = cat.id
            JOIN class_subcategories sub ON c.subcategory_id = sub.id
            ORDER BY c.is_racial DESC, cat.name, c.name
        """)
        columns = ['id', 'name', 'is_racial', 'category', 'subcategory']
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    finally:
        conn.close()

def get_all_categories() -> List[Dict]:
    """Get all class categories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM class_categories ORDER BY name")
        return [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def get_all_subcategories() -> List[Dict]:
    """Get all class subcategories"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT id, name FROM class_subcategories ORDER BY name")
        return [{'id': row[0], 'name': row[1]} for row in cursor.fetchall()]
    finally:
        conn.close()

def create_new_race(
    name: str,
    category_id: int,
    description: str,
    base_stats: Dict[str, int],
    per_level_stats: Dict[str, int],
    prerequisites: List[Dict],
    exclusions: List[Dict],
    never_obtainable: bool
) -> Tuple[bool, str, Optional[int]]:
    """Create a new race class with all its requirements and stats"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("BEGIN TRANSACTION")
        
        # Get subcategory for the race (Magekin)
        cursor.execute("SELECT id FROM class_subcategories WHERE name = 'Magekin' LIMIT 1")
        subcategory_id = cursor.fetchone()[0]

        # Get class type for the race (base)
        cursor.execute("SELECT id FROM class_types WHERE name = 'base' LIMIT 1")
        class_type_id = cursor.fetchone()[0]

        # Insert the race class
        cursor.execute("""
            INSERT INTO classes (
                name, description, class_type, is_racial, 
                category_id, subcategory_id,
                base_hp, base_mp, base_physical_attack, base_physical_defense,
                base_agility, base_magical_attack, base_magical_defense,
                base_resistance, base_special,
                hp_per_level, mp_per_level, physical_attack_per_level,
                physical_defense_per_level, agility_per_level, magical_attack_per_level,
                magical_defense_per_level, resistance_per_level, special_per_level
            ) VALUES (?, ?, ?, TRUE, ?, ?,
                     ?, ?, ?, ?, ?, ?, ?, ?, ?,
                     ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            name, description, class_type_id, category_id, subcategory_id,
            base_stats['hp'], base_stats['mp'], base_stats['physical_attack'],
            base_stats['physical_defense'], base_stats['agility'],
            base_stats['magical_attack'], base_stats['magical_defense'],
            base_stats['resistance'], base_stats['special'],
            per_level_stats['hp'], per_level_stats['mp'], per_level_stats['physical_attack'],
            per_level_stats['physical_defense'], per_level_stats['agility'],
            per_level_stats['magical_attack'], per_level_stats['magical_defense'],
            per_level_stats['resistance'], per_level_stats['special']
        ))
        
        class_id = cursor.lastrowid
        
        # Add prerequisites if any
        for group_idx, prereq_group in enumerate(prerequisites):
            for prereq in prereq_group:
                cursor.execute("""
                    INSERT INTO class_prerequisites (
                        class_id, prerequisite_group, prerequisite_type,
                        target_id, required_level, min_value, max_value
                    ) VALUES (?, ?, ?, ?, ?, ?, ?)
                """, (
                    class_id, group_idx, prereq['type'],
                    prereq.get('target_id'), prereq.get('required_level'),
                    prereq.get('min_value'), prereq.get('max_value')
                ))
        
        # Add exclusions if any
        for excl in exclusions:
            cursor.execute("""
                INSERT INTO class_exclusions (
                    class_id, exclusion_type, target_id,
                    min_value, max_value
                ) VALUES (?, ?, ?, ?, ?)
            """, (
                class_id, excl['type'], excl.get('target_id'),
                excl.get('min_value'), excl.get('max_value')
            ))
        
        # If never obtainable, add racial_total exclusion
        if never_obtainable:
            cursor.execute("""
                INSERT INTO class_exclusions (
                    class_id, exclusion_type, min_value
                ) VALUES (?, 'racial_total', 1)
            """, (class_id,))
        
        cursor.execute("COMMIT")
        return True, "Race created successfully!", class_id
        
    except Exception as e:
        cursor.execute("ROLLBACK")
        return False, f"Error creating race: {str(e)}", None
    finally:
        conn.close()

def render_prerequisites_section():
    """Render the prerequisites section with support for OR groups"""
    st.subheader("Prerequisites")
    
    if 'prereq_groups' not in st.session_state:
        st.session_state.prereq_groups = []
    
    # Add new group button
    if st.button("Add Prerequisite Group (OR)"):
        st.session_state.prereq_groups.append([])
    
    prerequisites = []
    
    # Render each group
    for group_idx, prereq_group in enumerate(st.session_state.prereq_groups):
        st.write(f"Group {group_idx + 1} (Any of these)")
        
        col1, col2 = st.columns([3, 1])
        with col1:
            # Map display names to database values
            prereq_type_mapping = {
                "Class": "specific_class",
                "Category Total": "category_total",
                "Subcategory Total": "subcategory_total",
                "Karma": "karma",
                "Quest": "quest",
                "Achievement": "achievement"
            }
            
            prereq_type_display = st.selectbox(
                "Type",
                list(prereq_type_mapping.keys()),
                key=f"prereq_type_{group_idx}"
            )
            prereq_type = prereq_type_mapping[prereq_type_display]
        
        with col2:
            if st.button("Add Requirement", key=f"add_req_{group_idx}"):
                st.session_state.prereq_groups[group_idx].append({
                    'type': prereq_type,
                    'target_id': None,
                    'required_level': None,
                    'min_value': None,
                    'max_value': None
                })
        
        # Render requirements in this group
        for req_idx, req in enumerate(prereq_group):
            st.markdown("---")
            cols = st.columns([3, 2, 1, 1, 1])
            
            with cols[0]:
                if req['type'] == "specific_class":
                    classes = get_all_classes()
                    options = [f"{c['name']} ({c['category']})" for c in classes]
                    selected = st.selectbox(
                        "Class",
                        range(len(options)),
                        format_func=lambda x: options[x],
                        key=f"class_{group_idx}_{req_idx}"
                    )
                    if selected is not None:
                        req['target_id'] = classes[selected]['id']
                        st.number_input(
                            "Required Level",
                            min_value=1,
                            value=req.get('required_level', 1),
                            key=f"level_{group_idx}_{req_idx}"
                        )
                
                elif req['type'] in ["category_total", "subcategory_total"]:
                    items = get_all_categories() if req['type'] == "Category Total" else get_all_subcategories()
                    options = [item['name'] for item in items]
                    selected = st.selectbox(
                        req['type'],
                        range(len(options)),
                        format_func=lambda x: options[x],
                        key=f"cat_{group_idx}_{req_idx}"
                    )
                    if selected is not None:
                        req['target_id'] = items[selected]['id']
                        st.number_input(
                            "Required Total Levels",
                            min_value=1,
                            value=req.get('required_level', 1),
                            key=f"total_{group_idx}_{req_idx}"
                        )
                
                elif req['type'] == "karma":
                    req['min_value'] = st.number_input(
                        "Minimum Karma",
                        value=req.get('min_value', -1000),
                        key=f"karma_min_{group_idx}_{req_idx}"
                    )
                    req['max_value'] = st.number_input(
                        "Maximum Karma",
                        value=req.get('max_value', 1000),
                        key=f"karma_max_{group_idx}_{req_idx}"
                    )
                
                elif req['type'] in ["quest", "achievement"]:
                    # Placeholder for quest/achievement selection
                    st.write(f"[Placeholder for {req['type']} selection]")
            
            with cols[4]:
                if st.button("Remove", key=f"remove_{group_idx}_{req_idx}"):
                    st.session_state.prereq_groups[group_idx].pop(req_idx)
                    st.rerun()
        
        if st.button(f"Remove Group {group_idx + 1}"):
            st.session_state.prereq_groups.pop(group_idx)
            st.rerun()
    
    return prerequisites

def render_exclusions_section():
    """Render the exclusions section"""
    st.subheader("Exclusions")
    
    if 'exclusions' not in st.session_state:
        st.session_state.exclusions = []
    
    exclusions = []
    
    col1, col2 = st.columns([3, 1])
    with col1:
                    # Map display names to database values
            exclusion_type_mapping = {
                "Class": "specific_class",
                "Category Total": "category_total",
                "Subcategory Total": "subcategory_total",
                "Karma": "karma",
                "Racial Level Total": "racial_total"
            }
            
            exclusion_type_display = st.selectbox(
                "Type",
                list(exclusion_type_mapping.keys())
            )
            exclusion_type = exclusion_type_mapping[exclusion_type_display]
    
    with col2:
        if st.button("Add Exclusion"):
            st.session_state.exclusions.append({
                'type': exclusion_type,
                'target_id': None,
                'min_value': None,
                'max_value': None
            })
    
    for idx, excl in enumerate(st.session_state.exclusions):
        st.markdown("---")
        cols = st.columns([3, 2, 1])
        
        with cols[0]:
            if excl['type'] == "Class":
                classes = get_all_classes()
                options = [f"{c['name']} ({c['category']})" for c in classes]
                selected = st.selectbox(
                    "Excluded Class",
                    range(len(options)),
                    format_func=lambda x: options[x],
                    key=f"excl_class_{idx}"
                )
                if selected is not None:
                    excl['target_id'] = classes[selected]['id']
            
            elif excl['type'] in ["Category Total", "Subcategory Total"]:
                items = get_all_categories() if excl['type'] == "Category Total" else get_all_subcategories()
                options = [item['name'] for item in items]
                selected = st.selectbox(
                    f"Excluded {excl['type']}",
                    range(len(options)),
                    format_func=lambda x: options[x],
                    key=f"excl_cat_{idx}"
                )
                if selected is not None:
                    excl['target_id'] = items[selected]['id']
                    excl['min_value'] = st.number_input(
                        "Minimum Levels",
                        value=excl.get('min_value', 1),
                        key=f"excl_min_{idx}"
                    )
            
            elif excl['type'] == "Karma":
                excl['min_value'] = st.number_input(
                    "Min Karma",
                    value=excl.get('min_value', -1000),
                    key=f"excl_karma_min_{idx}"
                )
                excl['max_value'] = st.number_input(
                    "Max Karma",
                    value=excl.get('max_value', 1000),
                    key=f"excl_karma_max_{idx}"
                )
        
        with cols[2]:
            if st.button("Remove", key=f"remove_excl_{idx}"):
                st.session_state.exclusions.pop(idx)
                st.rerun()
    
    return exclusions

def render_stats_section():
    """Render the base and per-level stats section"""
    col1, col2 = st.columns(2)
    
    base_stats = {}
    per_level_stats = {}
    
    with col1:
        st.subheader("Base Stats")
        base_stats['hp'] = st.number_input("Base HP", value=10)
        base_stats['mp'] = st.number_input("Base MP", value=5)
        base_stats['physical_attack'] = st.number_input("Base Physical Attack", value=5)
        base_stats['physical_defense'] = st.number_input("Base Physical Defense", value=5)
        base_stats['agility'] = st.number_input("Base Agility", value=5)
        base_stats['magical_attack'] = st.number_input("Base Magical Attack", value=5)
        base_stats['magical_defense'] = st.number_input("Base Magical Defense", value=5)
        base_stats['resistance'] = st.number_input("Base Resistance", value=5)
        base_stats['special'] = st.number_input("Base Special", value=5)
    
    with col2:
        st.subheader("Per Level Stats")
        per_level_stats['hp'] = st.number_input("HP per Level", value=2)
        per_level_stats['mp'] = st.number_input("MP per Level", value=1)
        per_level_stats['physical_attack'] = st.number_input("Physical Attack per Level", value=1)
        per_level_stats['physical_defense'] = st.number_input("Physical Defense per Level", value=1)
        per_level_stats['agility'] = st.number_input("Agility per Level", value=1)
        per_level_stats['magical_attack'] = st.number_input("Magical Attack per Level", value=1)
        per_level_stats['magical_defense'] = st.number_input("Magical Defense per Level", value=1)
        per_level_stats['resistance'] = st.number_input("Resistance per Level", value=1)
        per_level_stats['special'] = st.number_input("Special per Level", value=1)
    
    return base_stats, per_level_stats

def verify_race_exists(name: str) -> bool:
    """Verify that a race exists in the database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT COUNT(*) FROM classes 
            WHERE name = ? AND is_racial = TRUE
        """, (name,))
        count = cursor.fetchone()[0]
        return count > 0
    finally:
        conn.close()

def render_race_creation_form():
    """Render form for creating a new race"""
    st.subheader("Create New Race")
    
    # Initialize session state for banner visibility if not exists
    if 'show_success_banner' not in st.session_state:
        st.session_state.show_success_banner = False
    if 'success_race_name' not in st.session_state:
        st.session_state.success_race_name = ""
    
    # Get race categories
    race_categories = get_available_race_categories()
    
    # Create two columns - one for form, one for success banner
    form_col, banner_col = st.columns([2, 1])
    
    with form_col:
        # Basic Information
        st.subheader("Basic Information")
        name = st.text_input("Race Name")
        category_id = st.selectbox(
            "Race Category",
            options=[cat[0] for cat in race_categories],
            format_func=lambda x: next(cat[1] for cat in race_categories if cat[0] == x)
        )
        description = st.text_area("Description")
        
        # Stats
        st.markdown("---")
        st.subheader("Stats")
        base_stats, per_level_stats = render_stats_section()
        
        # Prerequisites
        st.markdown("---")
        prerequisites = render_prerequisites_section()
        
        # Exclusions
        st.markdown("---")
        exclusions = render_exclusions_section()
        
        # Obtainability
        st.markdown("---")
        st.subheader("Obtainability")
        never_obtainable = st.checkbox(
            "Never Obtainable In-Game", 
            help="If checked, this race can only be assigned during character creation or via special means"
        )
        
        if st.button("Create Race"):
            if name and category_id:
                success, message, class_id = create_new_race(
                    name=name,
                    category_id=category_id,
                    description=description,
                    base_stats=base_stats,
                    per_level_stats=per_level_stats,
                    prerequisites=st.session_state.prereq_groups,
                    exclusions=st.session_state.exclusions,
                    never_obtainable=never_obtainable
                )
                
                if success:
                    # Verify the race was actually added
                    if verify_race_exists(name):
                        st.session_state.show_success_banner = True
                        st.session_state.success_race_name = name
                        # Clear prerequisite and exclusion state
                        st.session_state.prereq_groups = []
                        st.session_state.exclusions = []
                        st.rerun()
                    else:
                        st.error("Race creation appeared successful but verification failed. Please try again.")
                else:
                    st.error(message)
            else:
                st.error("Name and category are required!")
    
    # Show success banner in the right column if active
    with banner_col:
        if st.session_state.show_success_banner:
            st.markdown("""
            <div style="padding: 1rem; background-color: #28a745; border-radius: 0.5rem; margin: 1rem 0;">
                <p style="color: white; font-size: 1.1rem; margin: 0;">
                    ✅ Race Successfully Added!
                </p>
            </div>
            """, unsafe_allow_html=True)
            
            # Add race details under the banner
            st.markdown(f"**{st.session_state.success_race_name}** has been added to the database.")
            
            # Schedule the banner to disappear
            st.session_state.show_success_banner = False