# ./SpellManager/spellEffectsManager.py

import streamlit as st
from .spellEffects import (
    load_effect_types,
    load_effects,
    save_effect,
    load_spell_effects,
    save_spell_effects
)

def render_spell_effects_manager():
    """Main interface for managing spell effects"""
    st.header("Spell Effects Manager")
    
    effects = load_effects()
    
    # Create tabs
    list_tab, editor_tab = st.tabs(["Effects List", "Effect Editor"])
    
    with list_tab:
        if effects:
            for idx, effect in enumerate(effects):
                with st.expander(f"{effect['name']} ({effect['type_name']})"):
                    col1, col2 = st.columns(2)
                    with col1:
                        st.write(f"Base Value: {effect['base_value']}")
                        st.write(f"Scaling: {effect['value_scaling']}")
                        st.write(f"Duration: {effect['duration']}")
                        st.write(f"Tick Type: {effect['tick_type']}")
                    with col2:
                        if st.button("Edit", key=f"edit_effect_{idx}"):
                            st.session_state.editing_effect = effect
                            st.rerun()
                        if st.button("Delete", key=f"delete_effect_{idx}"):
                            if save_effect({**effect, 'deleted': True}):
                                st.rerun()
    
    with editor_tab:
        effect_types = load_effect_types()
        
        if st.button("Create New Effect", key="create_new_effect"):
            st.session_state.editing_effect = {}
        
        if st.session_state.get('editing_effect') is not None:
            with st.form(key="spell_effects_manager_form"):
                effect_data = st.session_state.editing_effect
                
                name = st.text_input("Effect Name", value=effect_data.get('name', ''))
                
                type_options = {t['id']: t['name'] for t in effect_types}
                effect_type = st.selectbox(
                    "Effect Type",
                    options=list(type_options.keys()),
                    format_func=lambda x: type_options[x],
                    index=0 if not effect_data.get('effect_type_id') else 
                          list(type_options.keys()).index(effect_data['effect_type_id'])
                )
                
                col1, col2 = st.columns(2)
                with col1:
                    base_value = st.number_input(
                        "Base Value",
                        value=effect_data.get('base_value', 0)
                    )
                    value_scaling = st.text_input(
                        "Value Scaling",
                        value=effect_data.get('value_scaling', '')
                    )
                
                with col2:
                    duration = st.number_input(
                        "Duration (turns)",
                        min_value=0,
                        value=effect_data.get('duration', 0)
                    )
                    tick_type = st.selectbox(
                        "Tick Type",
                        options=['immediate', 'start_of_turn', 'end_of_turn'],
                        index=['immediate', 'start_of_turn', 'end_of_turn'].index(
                            effect_data.get('tick_type', 'immediate')
                        )
                    )
                
                description = st.text_area(
                    "Description",
                    value=effect_data.get('description', '')
                )
                
                if st.form_submit_button("Save Effect"):
                    effect = {
                        'id': effect_data.get('id'),
                        'name': name,
                        'effect_type_id': effect_type,
                        'base_value': base_value,
                        'value_scaling': value_scaling,
                        'duration': duration,
                        'tick_type': tick_type,
                        'description': description
                    }
                    
                    if save_effect(effect):
                        st.success("Effect saved!")
                        st.session_state.editing_effect = None
                        st.rerun()