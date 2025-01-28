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
from .spellStates import (
    load_spell_states,
    save_spell_state,
    load_character_spell_states,
    save_character_spell_state,
    remove_character_spell_state
)

def render_spell_states_section():
    """Render the spell states management section"""
    st.header("Spell States")
    
    # Load existing spell states
    states = load_spell_states()
    
    # Display existing states
    if states:
        for state in states:
            with st.expander(f"{state['name']} ({state['max_stacks']} max stacks)"):
                col1, col2 = st.columns(2)
                with col1:
                    st.write(f"Description: {state['description']}")
                    st.write(f"Duration: {state['duration'] or 'Until consumed'}")
                
                with col2:
                    if st.button(f"Edit {state['name']}"):
                        st.session_state.editing_state = state
                        st.rerun()
    
    # Add new state button
    if st.button("Add New State"):
        st.session_state.editing_state = {}
        st.rerun()
    
    # State editor form
    if st.session_state.get('editing_state') is not None:
        with st.form("state_editor_form"):
            state_data = st.session_state.editing_state
            
            name = st.text_input(
                "State Name",
                value=state_data.get('name', '')
            )
            
            description = st.text_area(
                "Description",
                value=state_data.get('description', '')
            )
            
            max_stacks = st.number_input(
                "Maximum Stacks",
                min_value=1,
                value=state_data.get('max_stacks', 1)
            )
            
            duration = st.number_input(
                "Duration (turns)",
                min_value=0,
                value=state_data.get('duration', 0),
                help="0 for permanent until consumed"
            )
            
            if st.form_submit_button("Save State"):
                state = {
                    'id': state_data.get('id'),
                    'name': name,
                    'description': description,
                    'max_stacks': max_stacks,
                    'duration': duration
                }
                
                if save_spell_state(state):
                    st.success("State saved successfully!")
                    st.session_state.editing_state = None
                    st.rerun()

def render_character_states_section(character_id: Optional[int] = None):
    """Render the character spell states section"""
    if not character_id:
        st.info("Select a character to manage their spell states")
        return
    
    st.header("Character Spell States")
    
    # Load character's current states
    char_states = load_character_spell_states(character_id)
    
    # Display current states
    if char_states:
        for state in char_states:
            with st.expander(f"{state['name']} ({state['current_stacks']}/{state['max_stacks']} stacks)"):
                col1, col2, col3 = st.columns(3)
                
                with col1:
                    st.write(f"Duration: {state['remaining_duration'] or 'Until consumed'}")
                
                with col2:
                    new_stacks = st.number_input(
                        "Adjust Stacks",
                        min_value=0,
                        max_value=state['max_stacks'],
                        value=state['current_stacks'],
                        key=f"stacks_{state['id']}"
                    )
                    if new_stacks != state['current_stacks']:
                        state['current_stacks'] = new_stacks
                        save_character_spell_state(character_id, state)
                        st.rerun()
                
                with col3:
                    if st.button(f"Remove {state['name']}"):
                        remove_character_spell_state(character_id, state['id'])
                        st.rerun()
    
    # Add state to character
    st.subheader("Add State")
    available_states = load_spell_states()
    
    # Filter out states the character already has
    current_state_ids = {s['spell_state_id'] for s in char_states}
    available_states = [s for s in available_states if s['id'] not in current_state_ids]
    
    if available_states:
        with st.form("add_character_state_form"):
            state_options = {str(s['id']): s['name'] for s in available_states}
            
            selected_state = st.selectbox(
                "Select State",
                options=list(state_options.keys()),
                format_func=lambda x: state_options[x]
            )
            
            selected_state_data = next(s for s in available_states if str(s['id']) == selected_state)
            
            initial_stacks = st.number_input(
                "Initial Stacks",
                min_value=1,
                max_value=selected_state_data['max_stacks'],
                value=1
            )
            
            if selected_state_data['duration']:
                duration = st.number_input(
                    "Duration",
                    min_value=1,
                    value=selected_state_data['duration']
                )
            else:
                duration = None
            
            if st.form_submit_button("Add State"):
                state = {
                    'spell_state_id': int(selected_state),
                    'current_stacks': initial_stacks,
                    'remaining_duration': duration
                }
                
                if save_character_spell_state(character_id, state):
                    st.success("State added successfully!")
                    st.rerun()
    else:
        st.info("No additional states available to add")

def render_spell_effects_section(spell_id: Optional[int] = None):
    """Render the main spell effects section"""
    if not spell_id:
        st.info("Select or save a spell first to manage effects")
        return
        
    tabs = st.tabs(["Effects", "States", "Character States"])
    
    with tabs[0]:
        render_effect_editor(spell_id)
    
    with tabs[1]:
        render_spell_states_section()
    
    with tabs[2]:
        # This could be connected to a character selector
        render_character_states_section(st.session_state.get('selected_character_id'))

def render_effect_editor(spell_id: int):
    """Original effect editor functionality"""
    effect_types = load_effect_types()
    
    if st.button("Create New Effect"):
        st.session_state.editing_effect = {}
    
    if st.session_state.get('editing_effect') is not None:
        with st.form("effect_editor_form"):
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
    
    # Display existing effects for this spell
    effects = load_spell_effects(spell_id)
    
    if effects:
        st.subheader("Current Effects")
        for effect in effects:
            with st.expander(f"{effect['name']} ({effect['type_name']})"):
                col1, col2, col3 = st.columns(3)
                
                with col1:
                    st.write(f"Base Value: {effect['base_value']}")
                    st.write(f"Scaling: {effect['value_scaling']}")
                
                with col2:
                    st.write(f"Duration: {effect['duration']} turns")
                    st.write(f"Timing: {effect['tick_type']}")
                
                with col3:
                    if st.button(f"Edit {effect['name']}"):
                        st.session_state.editing_effect = effect
                        st.rerun()
                    
                    if st.button(f"Remove {effect['name']}"):
                        effects.remove(effect)
                        save_spell_effects(spell_id, effects)
                        st.rerun()
    
    # Add existing effect to spell
    available_effects = load_effects()
    if available_effects:
        current_effect_ids = {e['id'] for e in effects} if effects else set()
        available_effects = [e for e in available_effects if e['id'] not in current_effect_ids]
        
        if available_effects:
            st.subheader("Add Existing Effect")
            effect_options = {str(e['id']): f"{e['name']} ({e['type_name']})" 
                            for e in available_effects}
            
            with st.form("add_existing_effect_form"):
                selected_effect = st.selectbox(
                    "Select Effect",
                    options=list(effect_options.keys()),
                    format_func=lambda x: effect_options[x]
                )
                
                if st.form_submit_button("Add Effect"):
                    effect = next(e for e in available_effects 
                                if str(e['id']) == selected_effect)
                    if effects is None:
                        effects = []
                    effects.append(effect)
                    save_spell_effects(spell_id, effects)
                    st.rerun()