# ./SpellManager/forms.py

import sqlite3
import streamlit as st
import pandas as pd
from typing import Dict, List
from pathlib import Path

def get_db_connection():
    """Create database connection"""
    db_path = Path('rpg_data.db')
    return sqlite3.connect(db_path)

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
    conn = get_db_connection()
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT id, name, description, spell_tier, mp_cost,
               casting_time, range, area_of_effect, damage_base, damage_scaling,
               healing_base, healing_scaling, status_effects, duration,
               (SELECT tier_name FROM spell_tiers WHERE id = spells.spell_tier) as tier_name
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

def render_spell_editor_tab():
    """Main interface for the spell editor tab"""
    st.header("Spell Editor")
    
    selected_tab = st.radio("Choose View", ["Spell List", "Spell Editor"], horizontal=True, label_visibility="hidden")
    
    if selected_tab == "Spell List":
        spells = load_spells()
        
        if spells:
            col1, col2, col3 = st.columns(3)
            with col1:
                if st.button("Add New Spell"):
                    st.session_state.editing_spell = {}
                    st.session_state.selected_tab = "Spell Editor"
                    st.rerun()
                    
            with col2:
                if st.button("Copy Selected Spell"):
                    selected_rows = [i for i, row in st.session_state.spell_table.get('edited_rows', {}).items() 
                                   if row.get('Select', False)]
                    if selected_rows:
                        spell_to_copy = spells[selected_rows[0]]
                        copied_spell = spell_to_copy.copy()
                        copied_spell.pop('id')
                        copied_spell['name'] = f"{copied_spell['name']} (Copy)"
                        st.session_state.editing_spell = copied_spell
                        st.session_state.spell_editor_tab = 1
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
            df['Select'] = False
            
            display_columns = ['Select', 'id', 'name', 'tier_name', 'mp_cost', 'damage_base', 'healing_base']
            display_names = {
                'Select': 'Select',
                'id': 'ID',
                'name': 'Name',
                'tier_name': 'Tier',
                'mp_cost': 'MP Cost',
                'damage_base': 'Base Damage',
                'healing_base': 'Base Healing',
            }
            
            display_df = df[display_columns].rename(columns=display_names)
            
            edited_df = st.data_editor(
                display_df,
                hide_index=True,
                column_config={
                    "Select": st.column_config.CheckboxColumn(
                        "Select",
                        help="Select spell to edit",
                        required=True
                    )
                },
                disabled=["ID", "Name", "Tier", "MP Cost", "Base Damage", "Base Healing"],
                key="spell_table"
            )
            
            if st.session_state.spell_table and 'edited_rows' in st.session_state.spell_table:
                selected_rows = [i for i, row in st.session_state.spell_table['edited_rows'].items() 
                               if row.get('Select', False)]
                if selected_rows:
                    selected_spell = spells[selected_rows[0]]
                    st.session_state.editing_spell = selected_spell
                    st.session_state.spell_editor_tab = 1
                    st.rerun()
        else:
            st.info("No spells found. Create one in the editor tab!")
    
    else:  # Spell Editor tab
        spell_data = st.session_state.get('editing_spell', {})
        
        with st.form("spell_editor_form_tab"):
            name = st.text_input("Spell Name", value=spell_data.get('name', ''))
            
            spell_type = load_spell_type()
            spell_type_options = {st['id']: st['name'] for st in spell_type}
            spell_type_id = st.selectbox(
                "Spell Type",
                options=list(spell_type_options.keys()),
                format_func=lambda x: spell_type_options[x],
                index=0 if not spell_data.get('spell_type_id') else 
                      list(spell_type_options.keys()).index(spell_data['spell_type_id'])
            )
            
            spell_tiers = load_spell_tiers()
            tier_options = {tier['id']: f"{tier['name']} - {tier['description']}" for tier in spell_tiers}
            spell_tier = st.selectbox(
                "Spell Tier",
                options=list(tier_options.keys()),
                format_func=lambda x: tier_options[x],
                index=0 if not spell_data.get('spell_tier') else 
                      list(tier_options.keys()).index(spell_data['spell_tier'])
            )
            
            description = st.text_area("Description", value=spell_data.get('description', ''))
            
            col1, col2 = st.columns(2)
            with col1:
                mp_cost = st.number_input("MP Cost", min_value=0, 
                                        value=spell_data.get('mp_cost', 0))
                casting_time = st.text_input("Casting Time", 
                                           value=spell_data.get('casting_time', ''))
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
                    'spell_type_id': spell_type_id,
                    'description': description,
                    'spell_tier': spell_tier,
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
                    st.session_state.spell_editor_tab = 0
                    st.rerun()