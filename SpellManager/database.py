# ./SpellManager/database.py

import sqlite3
import streamlit as st
from typing import Dict, List
from pathlib import Path

def get_db_connection():
    """Create database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def load_spells_with_query(query: str) -> List[Dict]:
    """Load spells using custom SQL query"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute(query)
        columns = [col[0] for col in cursor.description]
        spells = [dict(zip(columns, row)) for row in cursor.fetchall()]
        return spells
    finally:
        conn.close()

def load_spell_tiers() -> List[Dict]:
    """Load all spell tiers from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    cursor.execute("SELECT id, tier_name, description FROM spell_tiers ORDER BY id")
    
    tiers = [{"id": row[0], "name": row[1], "description": row[2]} for row in cursor.fetchall()]
    conn.close()
    return tiers

def load_spell_type() -> List[Dict]:
    """Load all spell types from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    cursor.execute("SELECT id, name FROM spell_type ORDER BY name")
    
    spell_type = [{"id": row[0], "name": row[1]} for row in cursor.fetchall()]
    conn.close()
    return spell_type

def load_spells() -> List[Dict]:
    """Load all spells from database"""
    default_query = """
        SELECT id, name, description, spell_tier, mp_cost,
               casting_time, range, area_of_effect, damage_base, damage_scaling,
               healing_base, healing_scaling, status_effects, duration,
               (SELECT tier_name FROM spell_tiers WHERE id = spells.spell_tier) as tier_name
        FROM spells
        ORDER BY spell_tier, name
    """
    return load_spells_with_query(default_query)

def save_spell(spell_data: Dict) -> bool:
    """Save spell to database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if 'id' in spell_data and spell_data['id']:
            cursor.execute("""
                UPDATE spells 
                SET name=?, spell_type_id=?, description=?, spell_tier=?, mp_cost=?,
                    casting_time=?, range=?, area_of_effect=?, damage_base=?,
                    damage_scaling=?, healing_base=?, healing_scaling=?,
                    status_effects=?, duration=?
                WHERE id=?
            """, (
                spell_data['name'], spell_data['spell_type_id'], spell_data['description'], 
                spell_data['spell_tier'], spell_data['mp_cost'], spell_data['casting_time'],
                spell_data['range'], spell_data['area_of_effect'], spell_data['damage_base'],
                spell_data['damage_scaling'], spell_data['healing_base'], 
                spell_data['healing_scaling'], spell_data['status_effects'],
                spell_data['duration'], spell_data['id']
            ))
        else:
            cursor.execute("""
                INSERT INTO spells (
                    name, spell_type_id, description, spell_tier, mp_cost,
                    casting_time, range, area_of_effect, damage_base, damage_scaling,
                    healing_base, healing_scaling, status_effects, duration
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                spell_data['name'], spell_data['spell_type_id'], spell_data['description'], 
                spell_data['spell_tier'], spell_data['mp_cost'], spell_data['casting_time'],
                spell_data['range'], spell_data['area_of_effect'], spell_data['damage_base'],
                spell_data['damage_scaling'], spell_data['healing_base'],
                spell_data['healing_scaling'], spell_data['status_effects'],
                spell_data['duration']
            ))
            
        conn.commit()
        return True
        
    except Exception as e:
        st.error(f"Error saving spell: {str(e)}")
        return False
        
    finally:
        conn.close()

def delete_spell(spell_id: int) -> bool:
    """Delete spell from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        cursor.execute("DELETE FROM spells WHERE id=?", (spell_id,))
        conn.commit()
        return True
    except Exception as e:
        st.error(f"Error deleting spell: {str(e)}")
        return False
    finally:
        conn.close()

# ./SpellManager/spellList.py

import streamlit as st
import pandas as pd
from .database import load_spells, delete_spell, load_spells_with_query

def render_spell_list():
    """Interface for viewing and managing the spell list"""
    
    # Add SQL query input field
    default_query = """
        SELECT id, name, tier_name, mp_cost, damage_base, healing_base 
        FROM spells 
        JOIN spell_tiers ON spells.spell_tier = spell_tiers.id
        ORDER BY spell_tier, name
    """
    
    query = st.text_area(
        "Custom SQL Query",
        value=default_query,
        help="Enter a custom SQL query to filter spells. Must include id, name fields."
    )
    
    # Add a separate button for executing the query
    col1, col2 = st.columns([1, 5])
    with col1:
        execute_query = st.button("Execute Query")
    with col2:
        if execute_query:
            try:
                spells = load_spells_with_query(query)
            except Exception as e:
                st.error(f"Error executing query: {str(e)}")
                spells = load_spells()  # Fallback to default loading
        else:
            spells = load_spells()
    
    if spells:
        col1, col2, col3 = st.columns(3)
        with col1:
            if st.button("Add New Spell"):
                st.session_state.editing_spell = {}
                st.session_state.spell_manager_tab = "Spell Editor"
                st.rerun()
                
        with col2:
            if st.button("Copy Selected Spell"):
                selected_rows = list(st.session_state.spell_table.get('edited_rows', {}).keys())
                if selected_rows:
                    spell_to_copy = spells[selected_rows[0]]
                    copied_spell = spell_to_copy.copy()
                    copied_spell.pop('id')
                    copied_spell['name'] = f"{copied_spell['name']} (Copy)"
                    st.session_state.editing_spell = copied_spell
                    st.session_state.spell_manager_tab = "Spell Editor"
                    st.rerun()
                else:
                    st.warning("Please select a spell to copy")
                    
        with col3:
            if st.button("Delete Selected Spell"):
                selected_rows = [i for i, row in st.session_state.spell_table.get('edited_rows', {}).items() 
                               if row.get('Select', False)]
                if selected_rows:
                    spell_to_delete = spells[selected_rows[0]]
                    if delete_spell(spell_to_delete['id']):
                        st.success("Spell deleted successfully!")
                        st.rerun()
                else:
                    st.warning("Please select a spell to delete")
        
        df = pd.DataFrame(spells)
        
        # Dynamically get display columns from the query results
        display_columns = df.columns.tolist()
        display_names = {
            'id': 'ID',
            'name': 'Name',
            'tier_name': 'Tier',
            'mp_cost': 'MP Cost',
            'damage_base': 'Base Damage',
            'healing_base': 'Base Healing',
        }
        
        # Only rename columns that have a display name mapping
        display_df = df.rename(columns={col: display_names.get(col, col) for col in display_columns})
        
        edited_df = st.data_editor(
            display_df,
            hide_index=True,
            disabled=display_columns,
            key="spell_table"
        )
        
        if st.session_state.spell_table and 'edited_rows' in st.session_state.spell_table:
            selected_rows = [i for i, row in st.session_state.spell_table['edited_rows'].items() 
                           if row.get('Select', False)]
            if selected_rows:
                selected_spell = spells[selected_rows[0]]
                st.session_state.editing_spell = selected_spell
                st.session_state.spell_manager_tab = "Spell Editor"
                st.rerun()
    else:
        st.info("No spells found. Create one in the editor tab!")