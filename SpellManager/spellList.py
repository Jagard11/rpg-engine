# ./SpellManager/spellList.py

import streamlit as st
import pandas as pd
from .database import load_spells, delete_spell, load_spells_with_query, save_spell

def render_spell_list():
    """Interface for viewing and managing the spell list"""
    
    # Check if query should be executed
    if st.session_state.get('execute_query'):
        try:
            spells = load_spells_with_query(st.session_state.spell_query)
        except Exception as e:
            st.error(f"Error executing query: {str(e)}")
            spells = load_spells()  # Fallback to default loading
    else:
        spells = load_spells()
    
    col1, col2, col3 = st.columns(3)
    with col1:
        if st.button("Add New Spell", key="add_new_spell_btn"):
            print("Creating new spell...")  # Debug output
            
            # Create initial spell data
            new_spell = {
                'name': 'New Spell',
                'description': '',
                'spell_tier': 1,  # Default to tier 1
                'is_super_tier': False,
                'mp_cost': 0,
                'casting_time': '',
                'range': '',
                'area_of_effect': '',
                'damage_base': 0,
                'damage_scaling': '',
                'healing_base': 0,
                'healing_scaling': '',
                'status_effects': '',
                'duration': ''
            }
            
            # Save to database to get ID
            new_id = save_spell(new_spell)
            if new_id:
                print(f"Created spell with ID: {new_id}")  # Debug output
                new_spell['id'] = new_id
                st.session_state.editing_spell = new_spell
                st.session_state.spell_manager_tab = "Spell Editor"
                st.rerun()
            else:
                st.error("Failed to create new spell")
            
    with col2:
        if st.button("Copy Selected Spell", key="copy_spell_btn"):
            if 'spell_table' in st.session_state and 'edited_rows' in st.session_state.spell_table:
                selected_rows = [i for i, row in st.session_state.spell_table['edited_rows'].items() 
                               if row.get('Select', False)]
                if selected_rows:
                    spell_to_copy = spells[selected_rows[0]]
                    copied_spell = spell_to_copy.copy()
                    copied_spell.pop('id', None)  # Remove ID to ensure it's treated as new
                    copied_spell['name'] = f"{copied_spell['name']} (Copy)"
                    
                    # Save copy to database
                    new_id = save_spell(copied_spell)
                    if new_id:
                        copied_spell['id'] = new_id
                        st.session_state.editing_spell = copied_spell
                        st.session_state.spell_manager_tab = "Spell Editor"
                        st.rerun()
                    else:
                        st.error("Failed to create spell copy")
                else:
                    st.warning("Please select a spell to copy")
                    
    with col3:
        if st.button("Delete Selected Spell", key="delete_spell_btn"):
            if 'spell_table' in st.session_state and 'edited_rows' in st.session_state.spell_table:
                selected_rows = [i for i, row in st.session_state.spell_table['edited_rows'].items() 
                               if row.get('Select', False)]
                if selected_rows:
                    spell_to_delete = spells[selected_rows[0]]
                    if delete_spell(spell_to_delete['id']):
                        st.success("Spell deleted successfully!")
                        st.rerun()
                else:
                    st.warning("Please select a spell to delete")
    
    if spells:
        df = pd.DataFrame(spells)
        
        # Add selection column
        if 'Select' not in df.columns:
            df.insert(0, 'Select', False)
        
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
            disabled=[col for col in display_columns if col != 'Select'],
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
        st.info("No spells found. Create one using the Add New Spell button!")