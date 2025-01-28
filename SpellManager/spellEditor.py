# ./SpellManager/spellEditor.py

import streamlit as st
import json
from .database import load_spell_tiers, load_spell_type, save_spell, load_spells_with_query

def render_spell_editor():
    """Interface for editing individual spells"""
    spell_data = st.session_state.get('editing_spell', {})
    
    with st.form("spell_editor_component_form"):
        # Create columns for ID and name
        col_id, col_name = st.columns([1, 4])
        
        with col_id:
            # Display current record or "0" if creating new
            if spell_data.get('id'):
                query = """
                    SELECT 
                        s.*,
                        st.tier_name
                    FROM spells s
                    LEFT JOIN spell_tiers st ON s.spell_tier = st.id
                    WHERE s.id = {}
                """.format(spell_data['id'])
                current_record = load_spells_with_query(query)
                if current_record:
                    formatted_record = str(current_record[0]["id"])
                else:
                    formatted_record = "Error"
            else:
                formatted_record = "0"
                
            st.text_input(
                "Record ID",
                value=formatted_record,
                disabled=True
            )
            
        with col_name:
            name = st.text_input("Spell Name", value=spell_data.get('name', ''))
        
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
                st.session_state.spell_manager_tab = "Spell List"
                st.rerun()