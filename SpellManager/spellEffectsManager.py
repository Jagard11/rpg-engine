# ./SpellManager/spellEffectsManager.py

import pandas as pd

import streamlit as st
from .spellEffects import (
    load_effect_types,
    load_effects,
    save_effect,
    load_spell_effects,
    save_spell_effects
)
from .database import load_spells

def render_spell_effects_manager():
    """Main interface for managing spell effects"""
    st.header("Spell Effects Manager")

    # Create tabs for different aspects of effect management
    list_tab, editor_tab, assignment_tab = st.tabs([
        "Effects List", 
        "Effect Editor",
        "Spell Assignment"
    ])

    with list_tab:
        effects = load_effects()
        if effects:
            effect_df = pd.DataFrame(effects)
            st.dataframe(
                effect_df,
                column_config={
                    "id": "ID",
                    "name": "Name",
                    "type_name": "Type",
                    "base_value": "Base Value",
                    "duration": "Duration",
                    "tick_type": "Timing"
                },
                hide_index=True
            )
        else:
            st.info("No effects created yet. Use the editor to create some!")

    with editor_tab:
        # Effect editing interface
        effect_types = load_effect_types()
        
        if st.button("Create New Effect"):
            st.session_state.editing_effect = {}
        
        if st.session_state.get('editing_effect') is not None:
            with st.form("spell_effects_editor_form"):
                effect_data = st.session_state.editing_effect
                
                name = st.text_input(
                    "Effect Name", 
                    value=effect_data.get('name', '')
                )
                
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
                        value=effect_data.get('value_scaling', ''),
                        help="Formula for scaling with level/stats"
                    )
                
                with col2:
                    duration = st.number_input(
                        "Duration (turns)",
                        min_value=0,
                        value=effect_data.get('duration', 0)
                    )
                    tick_type = st.selectbox(
                        "Effect Timing",
                        options=['immediate', 'start_of_turn', 'end_of_turn'],
                        index=['immediate', 'start_of_turn', 'end_of_turn'].index(
                            effect_data.get('tick_type', 'immediate')
                        )
                    )
                
                description = st.text_area(
                    "Description",
                    value=effect_data.get('description', '')
                )
                
                submitted = st.form_submit_button("Save Effect")
                if submitted:
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
                        st.success("Effect saved successfully!")
                        st.session_state.editing_effect = None
                        st.rerun()

    with assignment_tab:
        # Spell-effect assignment interface
        spells = load_spells()
        if not spells:
            st.info("No spells found. Create some spells first!")
            return
            
        spell_options = {str(s['id']): f"{s['name']} (Tier {s['tier_name']})" 
                        for s in spells}
        
        selected_spell = st.selectbox(
            "Select Spell",
            options=list(spell_options.keys()),
            format_func=lambda x: spell_options[x]
        )
        
        if selected_spell:
            spell_id = int(selected_spell)
            current_effects = load_spell_effects(spell_id)
            
            st.subheader("Current Effects")
            for idx, effect in enumerate(current_effects):
                with st.expander(f"{effect['name']} ({effect['type_name']})"):
                    st.write(f"Base Value: {effect['base_value']}")
                    st.write(f"Scaling: {effect['value_scaling']}")
                    st.write(f"Duration: {effect['duration']} turns")
                    st.write(f"Timing: {effect['tick_type']}")
                    probability = st.slider(
                        "Probability",
                        min_value=0.0,
                        max_value=1.0,
                        value=effect.get('probability', 1.0),
                        step=0.1,
                        key=f"prob_{idx}"
                    )
                    effect['probability'] = probability
                    
                    if st.button(f"Remove Effect {idx}"):
                        current_effects.pop(idx)
                        save_spell_effects(spell_id, current_effects)
                        st.rerun()
            
            st.subheader("Add Effect")
            available_effects = load_effects()
            
            # Filter out effects already added to this spell
            current_effect_ids = {e['id'] for e in current_effects}
            available_effects = [e for e in available_effects 
                               if e['id'] not in current_effect_ids]
            
            if available_effects:
                effect_options = {str(e['id']): f"{e['name']} ({e['type_name']})" 
                                for e in available_effects}
                
                selected_effect = st.selectbox(
                    "Select Effect to Add",
                    options=list(effect_options.keys()),
                    format_func=lambda x: effect_options[x]
                )
                
                if selected_effect:
                    effect = next(e for e in available_effects 
                                if str(e['id']) == selected_effect)
                    probability = st.slider(
                        "Effect Probability",
                        min_value=0.0,
                        max_value=1.0,
                        value=1.0,
                        step=0.1
                    )
                    
                    if st.button("Add Effect"):
                        current_effects.append({
                            **effect,
                            'probability': probability
                        })
                        save_spell_effects(spell_id, current_effects)
                        st.rerun()
            else:
                st.info("All available effects have been added to this spell.")