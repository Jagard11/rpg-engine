# ./SpellManager/forms.py

import sqlite3
import streamlit as st
import pandas as pd
from typing import Dict, List, Optional
from pathlib import Path

def get_db_connection():
    """Create database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

def load_spells() -> List[Dict]:
    """Load all spells from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT id, name, description, spell_tier, is_super_tier, mp_cost, 
               casting_time, range, area_of_effect, damage_base, damage_scaling,
               healing_base, healing_scaling, status_effects, duration
        FROM spells
        ORDER BY spell_tier, name
    """)
    
    columns = [col[0] for col in cursor.description]
    spells = [dict(zip(columns, row)) for row in cursor.fetchall()]
    conn.close()
    return spells

def save_spell(spell_data: Dict) -> bool:
    """Save spell to database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    
    try:
        if 'id' in spell_data and spell_data['id']:
            cursor.execute("""
                UPDATE spells 
                SET name=?, description=?, spell_tier=?, is_super_tier=?, mp_cost=?,
                    casting_time=?, range=?, area_of_effect=?, damage_base=?,
                    damage_scaling=?, healing_base=?, healing_scaling=?,
                    status_effects=?, duration=?
                WHERE id=?
            """, (
                spell_data['name'], spell_data['description'], spell_data['spell_tier'],
                spell_data['is_super_tier'], spell_data['mp_cost'], spell_data['casting_time'],
                spell_data['range'], spell_data['area_of_effect'], spell_data['damage_base'],
                spell_data['damage_scaling'], spell_data['healing_base'], 
                spell_data['healing_scaling'], spell_data['status_effects'],
                spell_data['duration'], spell_data['id']
            ))
        else:
            cursor.execute("""
                INSERT INTO spells (
                    name, description, spell_tier, is_super_tier, mp_cost,
                    casting_time, range, area_of_effect, damage_base, damage_scaling,
                    healing_base, healing_scaling, status_effects, duration
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                spell_data['name'], spell_data['description'], spell_data['spell_tier'],
                spell_data['is_super_tier'], spell_data['mp_cost'], spell_data['casting_time'],
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

def render_spell_editor_tab():
    """Main interface for the spell editor tab"""
    st.header("Spell Editor")
    
    # Load existing spells
    spells = load_spells()
    
    # Create tabs for list view and editor
    list_tab, editor_tab = st.tabs(["Spell List", "Spell Editor"])
    
    with list_tab:
        if spells:
            # Convert spells to DataFrame for table display
            df = pd.DataFrame(spells)
            
            # Reorder and rename columns for display
            display_columns = ['name', 'spell_tier', 'mp_cost', 'is_super_tier', 'damage_base', 'healing_base']
            display_names = {
                'name': 'Name',
                'spell_tier': 'Tier',
                'mp_cost': 'MP Cost',
                'is_super_tier': 'Super Tier',
                'damage_base': 'Base Damage',
                'healing_base': 'Base Healing'
            }
            
            display_df = df[display_columns].rename(columns=display_names)
            
            # Create selectable table
            selected_indices = st.data_editor(
                display_df,
                hide_index=True,
                column_config={
                    "Super Tier": st.column_config.CheckboxColumn(
                        "Super Tier",
                        help="Whether this is a super tier spell",
                        default=False,
                    )
                },
                disabled=True,
                key="spell_table"
            )
            
            # Handle row selection
            if st.session_state.spell_table:
                selected_row = st.session_state.spell_table['edited_rows']
                if selected_row:
                    row_idx = list(selected_row.keys())[0]
                    selected_spell = spells[row_idx]
                    st.session_state.editing_spell = selected_spell
                    st.session_state.active_tab = "editor"
                    st.experimental_rerun()
        else:
            st.info("No spells found. Create one in the editor tab!")
    
    with editor_tab:
        spell_data = st.session_state.get('editing_spell', {})
        
        with st.form("spell_editor_form_tab"):
            name = st.text_input("Spell Name", value=spell_data.get('name', ''))
            description = st.text_area("Description", value=spell_data.get('description', ''))
            
            col1, col2 = st.columns(2)
            with col1:
                tier = st.number_input("Spell Tier", min_value=0, max_value=10, 
                                     value=spell_data.get('spell_tier', 0))
                mp_cost = st.number_input("MP Cost", min_value=0, 
                                        value=spell_data.get('mp_cost', 0))
                casting_time = st.text_input("Casting Time", 
                                           value=spell_data.get('casting_time', ''))
                is_super_tier = st.checkbox("Super Tier Spell", 
                                          value=spell_data.get('is_super_tier', False))
                range_val = st.text_input("Range", value=spell_data.get('range', ''))
                
            with col2:
                area = st.text_input("Area of Effect", 
                                   value=spell_data.get('area_of_effect', ''))
                duration = st.text_input("Duration", 
                                       value=spell_data.get('duration', ''))
            
            col3, col4 = st.columns(2)
            with col3:
                damage_base = st.number_input("Base Damage", min_value=0,
                                            value=spell_data.get('damage_base', 0))
                damage_scaling = st.text_input("Damage Scaling",
                                             value=spell_data.get('damage_scaling', ''))
                
            with col4:
                healing_base = st.number_input("Base Healing", min_value=0,
                                             value=spell_data.get('healing_base', 0))
                healing_scaling = st.text_input("Healing Scaling",
                                              value=spell_data.get('healing_scaling', ''))
                
            status_effects = st.text_area("Status Effects", 
                                        value=spell_data.get('status_effects', ''))
            
            submitted = st.form_submit_button("Save Spell")
            if submitted:
                spell_data = {
                    'id': spell_data.get('id'),
                    'name': name,
                    'description': description,
                    'spell_tier': tier,
                    'is_super_tier': is_super_tier,
                    'mp_cost': mp_cost,
                    'casting_time': casting_time,
                    'range': range_val,
                    'area_of_effect': area,
                    'damage_base': damage_base,
                    'damage_scaling': damage_scaling,
                    'healing_base': healing_base,
                    'healing_scaling': healing_scaling,
                    'status_effects': status_effects,
                    'duration': duration
                }
                
                if save_spell(spell_data):
                    st.success("Spell saved successfully!")
                    st.session_state.editing_spell = {}
                    st.experimental_rerun()