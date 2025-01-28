# ./SpellManager/spellList.py

import streamlit as st
import pandas as pd
from .database import load_spells, delete_spell

def render_spell_list():
    """Interface for viewing and managing the spell list"""
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
        
        display_columns = ['id', 'name', 'tier_name', 'mp_cost', 'damage_base', 'healing_base']
        display_names = {
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