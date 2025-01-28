# ./SpellManager/spellEditor.py

# Update imports to include effects manager
from .spellEffectsManager import render_spell_effects_manager

import streamlit as st
from .database import load_spell_tiers, load_spell_type, save_spell
from .spellEffectsEditor import render_spell_effects_section

def render_spell_editor():
    """Interface for editing individual spells"""
    spell_data = st.session_state.get('editing_spell', {})
    
    # Initialize effect editor state if needed
    if 'adding_new_effect' not in st.session_state:
        st.session_state.adding_new_effect = False
    if 'adding_existing_effect' not in st.session_state:
        st.session_state.adding_existing_effect = False
    if 'editing_effect' not in st.session_state:
        st.session_state.editing_effect = None
    
    with st.form("spell_base_editor_form"):
        # Basic spell info section
        st.subheader("Basic Information")
        col_id, col_name = st.columns([1, 4])
        
        with col_id:
            # Display record ID
            st.text_input(
                "Record ID",
                value=str(spell_data.get('id', '0')),
                disabled=True
            )
        
        with col_name:
            name = st.text_input("Spell Name", value=spell_data.get('name', ''))
        
        # Load and display spell types
        spell_types = load_spell_type()
        type_options = {str(t['id']): t['name'] for t in spell_types}
        
        spell_type = st.selectbox(
            "Spell Type",
            options=list(type_options.keys()),
            format_func=lambda x: type_options[x],
            index=0 if not spell_data.get('spell_type_id') else 
                  list(type_options.keys()).index(str(spell_data['spell_type_id']))
        )
        
        description = st.text_area("Description", value=spell_data.get('description', ''))
        
        # Spell tier selection
        spell_tiers = load_spell_tiers()
        tier_options = {tier['id']: f"{tier['tier_name']}" 
                       for tier in spell_tiers}
        
        spell_tier = st.selectbox(
            "Spell Tier",
            options=list(tier_options.keys()),
            format_func=lambda x: tier_options[x],
            index=0 if not spell_data.get('spell_tier') else 
                  list(tier_options.keys()).index(spell_data['spell_tier'])
        )
        
        # Basic spell stats
        st.subheader("Basic Stats")
        col1, col2 = st.columns(2)
        
        with col1:
            mp_cost = st.number_input(
                "MP Cost",
                min_value=0,
                value=spell_data.get('mp_cost', 0)
            )
            
            casting_time = st.text_input(
                "Casting Time",
                value=spell_data.get('casting_time', '')
            )
            
            range_val = st.text_input(
                "Range",
                value=spell_data.get('range', '')
            )
        
        with col2:
            area = st.text_input(
                "Area of Effect",
                value=spell_data.get('area_of_effect', '')
            )
            
            duration = st.text_input(
                "Duration",
                value=spell_data.get('duration', '')
            )
        
        # Save button
        submitted = st.form_submit_button("Save Spell")
        
        if submitted:
            spell_data = {
                'id': spell_data.get('id'),
                'name': name,
                'description': description,
                'spell_tier': spell_tier,
                'spell_type_id': int(spell_type),
                'mp_cost': mp_cost,
                'casting_time': casting_time,
                'range': range_val,
                'area_of_effect': area,
                'duration': duration
            }
            
            if save_spell(spell_data):
                st.success("Spell saved successfully!")
                # Keep editing the same spell
                st.session_state.editing_spell = spell_data
                st.rerun()
    
    # Effects section (outside the main form)
    if spell_data.get('id'):
        st.markdown("---")
        render_spell_effects_section(spell_data['id'])
    else:
        st.info("Save the spell first to add effects")