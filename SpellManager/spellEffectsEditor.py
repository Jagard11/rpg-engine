# ./SpellManager/spellEffectsEditor.py

import streamlit as st
from typing import List, Dict, Optional
from .spellEffects import (
    load_effect_types,
    load_effects,
    save_effect,
    load_spell_effects,
    save_spell_effects
)

def render_effect_editor(effect_data: Optional[Dict] = None) -> Dict:
    """Render the effect editor form"""
    effect_types = load_effect_types()
    
    with st.form("effect_editor_form"):
        name = st.text_input("Effect Name", value=effect_data.get('name', '') if effect_data else '')
        
        # Effect type selection
        type_options = {t['id']: t['name'] for t in effect_types}
        effect_type = st.selectbox(
            "Effect Type",
            options=list(type_options.keys()),
            format_func=lambda x: type_options[x],
            index=0 if not effect_data else 
                  list(type_options.keys()).index(effect_data['effect_type_id'])
        )
        
        col1, col2 = st.columns(2)
        with col1:
            base_value = st.number_input(
                "Base Value",
                value=effect_data.get('base_value', 0) if effect_data else 0
            )
            duration = st.number_input(
                "Duration (turns)",
                min_value=0,
                value=effect_data.get('duration', 0) if effect_data else 0,
                help="0 for immediate effects, >0 for effects that last multiple turns"
            )
        
        with col2:
            value_scaling = st.text_input(
                "Value Scaling",
                value=effect_data.get('value_scaling', '') if effect_data else '',
                help="Formula for how effect scales with level/stats"
            )
            tick_type = st.selectbox(
                "Tick Type",
                options=['immediate', 'start_of_turn', 'end_of_turn'],
                index=['immediate', 'start_of_turn', 'end_of_turn'].index(
                    effect_data.get('tick_type', 'immediate') if effect_data else 'immediate'
                )
            )
        
        description = st.text_area(
            "Description",
            value=effect_data.get('description', '') if effect_data else ''
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
            return effect
    
    return None

def render_spell_effects_section(spell_id: Optional[int] = None):
    """Render the effects section of the spell editor"""
    if not spell_id:
        st.warning("Save the spell first to add effects")
        return
    
    st.subheader("Spell Effects")
    
    # Load existing effects for this spell
    current_effects = load_spell_effects(spell_id)
    
    # Effect management buttons
    col1, col2 = st.columns(2)
    with col1:
        if st.button("Add New Effect"):
            st.session_state.adding_new_effect = True
    
    with col2:
        if st.button("Add Existing Effect"):
            st.session_state.adding_existing_effect = True
    
    # New effect form
    if st.session_state.get('adding_new_effect'):
        st.subheader("Create New Effect")
        new_effect = render_effect_editor()
        if new_effect:
            effect_id = save_effect(new_effect)
            if effect_id:
                current_effects.append({
                    **new_effect,
                    'id': effect_id,
                    'probability': 1.0
                })
                save_spell_effects(spell_id, current_effects)
                st.session_state.adding_new_effect = False
                st.rerun()
    
    # Existing effect selector
    if st.session_state.get('adding_existing_effect'):
        st.subheader("Add Existing Effect")
        available_effects = load_effects()
        
        # Filter out effects already added to this spell
        current_effect_ids = {e['id'] for e in current_effects}
        available_effects = [e for e in available_effects if e['id'] not in current_effect_ids]
        
        effect_options = {str(e['id']): f"{e['name']} ({e['type_name']})" for e in available_effects}
        
        selected_effect = st.selectbox(
            "Select Effect",
            options=list(effect_options.keys()),
            format_func=lambda x: effect_options[x]
        )
        
        probability = st.slider(
            "Probability",
            min_value=0.0,
            max_value=1.0,
            value=1.0,
            step=0.1,
            help="Chance of this effect being applied"
        )
        
        if st.button("Add Effect"):
            effect = next(e for e in available_effects if str(e['id']) == selected_effect)
            current_effects.append({**effect, 'probability': probability})
            save_spell_effects(spell_id, current_effects)
            st.session_state.adding_existing_effect = False
            st.rerun()
    
    # Display current effects
    if current_effects:
        st.subheader("Current Effects")
        for idx, effect in enumerate(current_effects):
            with st.expander(f"{effect['name']} ({effect['type_name']})"):
                st.write(f"Base Value: {effect['base_value']}")
                st.write(f"Scaling: {effect['value_scaling']}")
                st.write(f"Duration: {effect['duration']} turns")
                st.write(f"Tick Type: {effect['tick_type']}")
                st.write(f"Probability: {effect.get('probability', 1.0)*100}%")
                
                col1, col2 = st.columns(2)
                with col1:
                    if st.button(f"Edit Effect {idx}"):
                        st.session_state.editing_effect = None
                st.session_state.editing_effect = effect
                st.rerun()
                with col2:
                    if st.button(f"Remove Effect {idx}"):
                        current_effects.pop(idx)
                        save_spell_effects(spell_id, current_effects)
                        st.rerun()
    
    # Edit effect form
    if st.session_state.get('editing_effect'):
        st.subheader("Edit Effect")
        updated_effect = render_effect_editor(st.session_state.editing_effect)
        if updated_effect:
            effect_id = save_effect(updated_effect)
            if effect_id:
                # Update the effect in the current effects list
                for idx, effect in enumerate(current_effects):
                    if effect['id'] == effect_id:
                        current_effects[idx] = {**updated_effect, 'id': effect_id}
                        break
                save_spell_effects(spell_id, current_effects)
                st.session_state.editing