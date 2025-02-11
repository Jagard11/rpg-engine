# ./SpellManager/spellEffectEditor.py

import streamlit as st
from typing import Dict, Optional, List
from .database import (
    load_effects,
    save_effect,
    load_spell_effects,
    save_spell_effects,
    load_effect_types
)

def render_effect_editor(spell_id: Optional[int] = None):
    """Main interface for spell effect configuration"""
    if not spell_id:
        st.info("Please save the spell first to configure effects")
        return

    st.header("Effect Configuration")
    
    # Create tabs for different effect aspects
    select_tab, config_tab, chain_tab = st.tabs([
        "Effect Selection",
        "Effect Parameters",
        "Effect Chaining"
    ])
    
    with select_tab:
        render_effect_selection(spell_id)
        
    with config_tab:
        render_effect_parameters(spell_id)
        
    with chain_tab:
        render_effect_chaining(spell_id)

def render_effect_selection(spell_id: int):
    """Interface for selecting and creating effects"""
    st.subheader("Select Effects")
    
    # Load registered effects and current spell effects
    registered_effects = load_effects()
    current_effects = load_spell_effects(spell_id)
    
    # Display current effects
    if current_effects:
        st.write("Current Effects:")
        for idx, effect in enumerate(current_effects):
            with st.expander(f"{effect['effect_type']} Effect"):
                st.write(f"Base Value: {effect['base_value']}")
                st.write(f"Duration: {effect.get('duration', 0)} turns")
                st.write(f"Proc Chance: {effect.get('proc_chance', 1.0)}")
                if st.button("Remove Effect", key=f"remove_effect_{idx}"):
                    current_effects.remove(effect)
                    save_spell_effects(spell_id, current_effects)
                    st.rerun()
    
    # Add registered effect
    if registered_effects:
        with st.form("add_registered_effect_form"):
            st.write("Add Existing Effect:")
            
            # Create effect options, including the effect type name
            effect_options = {str(e['id']): f"{e['name']} ({e['type_name']})" 
                            for e in registered_effects}
            
            selected_effect = st.selectbox(
                "Select Effect",
                options=list(effect_options.keys()),
                format_func=lambda x: effect_options[x],
                key="registered_effect_select"
            )
            
            # Get the selected effect data
            effect_data = next(e for e in registered_effects 
                             if str(e['id']) == selected_effect)
            
            # Additional effect parameters
            col1, col2 = st.columns(2)
            with col1:
                base_value = st.number_input(
                    "Base Value",
                    value=effect_data.get('base_value', 0),
                    key="effect_base_value"
                )
                duration = st.number_input(
                    "Duration (turns)",
                    min_value=0,
                    value=effect_data.get('duration', 0),
                    key="effect_duration"
                )
            
            with col2:
                target_stat = st.selectbox(
                    "Target Stat",
                    options=["hp", "mp", "physical_attack", "magical_attack", "defense"],
                    key="effect_target_stat"
                )
                proc_chance = st.slider(
                    "Proc Chance",
                    min_value=0.0,
                    max_value=1.0,
                    value=1.0,
                    key="effect_proc_chance"
                )
            
            if st.form_submit_button("Add Effect"):
                # Create new effect entry
                new_effect = {
                    'effect_type': effect_data['type_name'],
                    'base_value': base_value,
                    'duration': duration,
                    'target_stat_id': 1,  # This will be mapped based on target_stat selection
                    'proc_chance': proc_chance,
                    'tick_rate': 1,
                    'scaling_formula': '',
                    'scaling_stat_id': None  # Initialize with no scaling stat
                }
                
                # Debug print
                print(f"Adding new effect: {new_effect}")
                
                if current_effects is None:
                    current_effects = []
                current_effects.append(new_effect)
                
                try:
                    if save_spell_effects(spell_id, current_effects):
                        st.success("Effect added successfully!")
                        st.rerun()
                    else:
                        st.error("Failed to add effect.")
                except Exception as e:
                    st.error(f"Error adding effect: {str(e)}")
                    print(f"Error details: {str(e)}")
    
    # Create new effect
    if st.button("Create New Effect", key="create_new_effect_btn"):
        st.session_state.creating_new_effect = True
        
    if st.session_state.get('creating_new_effect'):
        render_new_effect_form(spell_id)

def render_effect_parameters(spell_id: int):
    """Interface for configuring effect parameters"""
    st.subheader("Effect Parameters")
    
    current_effects = load_spell_effects(spell_id)
    if not current_effects:
        st.info("Add effects first to configure parameters")
        return
    
    for idx, effect in enumerate(current_effects):
        with st.expander(f"Configure {effect['effect_type']} Effect"):
            with st.form(f"effect_params_{idx}"):
                col1, col2 = st.columns(2)
                
                with col1:
                    base_value = st.number_input(
                        "Base Value",
                        value=effect['base_value'],
                        key=f"param_base_{idx}"
                    )
                    
                    scaling_formula = st.text_input(
                        "Scaling Formula",
                        value=effect.get('scaling_formula', ''),
                        key=f"param_scaling_{idx}"
                    )
                
                with col2:
                    duration = st.number_input(
                        "Duration (turns)",
                        min_value=0,
                        value=effect.get('duration', 0),
                        key=f"param_duration_{idx}"
                    )
                    
                    tick_rate = st.number_input(
                        "Tick Rate (turns)",
                        min_value=1,
                        value=effect.get('tick_rate', 1),
                        key=f"param_tick_{idx}"
                    )
                
                if st.form_submit_button("Update Parameters"):
                    effect.update({
                        'base_value': base_value,
                        'scaling_formula': scaling_formula,
                        'duration': duration,
                        'tick_rate': tick_rate
                    })
                    if save_spell_effects(spell_id, current_effects):
                        st.success("Parameters updated!")
                    else:
                        st.error("Failed to update parameters.")

def render_effect_chaining(spell_id: int):
    """Interface for chaining multiple effects"""
    st.subheader("Effect Chain Configuration")
    
    current_effects = load_spell_effects(spell_id)
    if not current_effects:
        st.info("Add effects first to configure chaining")
        return
    
    st.write("Drag effects to reorder the chain:")
    
    # Display effects in order with up/down buttons
    for idx, effect in enumerate(current_effects):
        col1, col2, col3 = st.columns([3, 1, 1])
        
        with col1:
            st.write(f"{idx+1}. {effect['effect_type']} Effect")
        
        with col2:
            if idx > 0 and st.button("↑", key=f"move_up_{idx}"):
                current_effects[idx], current_effects[idx-1] = current_effects[idx-1], current_effects[idx]
                if save_spell_effects(spell_id, current_effects):
                    st.rerun()
        
        with col3:
            if idx < len(current_effects)-1 and st.button("↓", key=f"move_down_{idx}"):
                current_effects[idx], current_effects[idx+1] = current_effects[idx+1], current_effects[idx]
                if save_spell_effects(spell_id, current_effects):
                    st.rerun()

def render_new_effect_form(spell_id: int):
    """Form for creating a new effect"""
    effect_types = load_effect_types()
    
    with st.form("new_effect_form"):
        st.write("Create New Effect")
        
        name = st.text_input("Effect Name", key="new_effect_name")
        
        type_options = {str(t['id']): t['name'] for t in effect_types}
        selected_type = st.selectbox(
            "Effect Type",
            options=list(type_options.keys()),
            format_func=lambda x: type_options[x],
            key="new_effect_type"
        )
        
        description = st.text_area("Description", key="new_effect_desc")
        
        col1, col2 = st.columns(2)
        
        with col1:
            base_value = st.number_input(
                "Base Value",
                value=0,
                key="new_effect_base"
            )
            duration = st.number_input(
                "Duration (turns)",
                min_value=0,
                value=0,
                key="new_effect_duration"
            )
        
        with col2:
            scaling_formula = st.text_input(
                "Scaling Formula",
                value="",
                key="new_effect_scaling"
            )
            tick_type = st.selectbox(
                "Tick Type",
                options=['immediate', 'start_of_turn', 'end_of_turn'],
                index=0,
                key="new_effect_tick"
            )
        
        if st.form_submit_button("Create Effect"):
            effect_type_id = int(selected_type)
            
            new_effect = {
                'name': name,
                'effect_type_id': effect_type_id,
                'description': description,
                'base_value': base_value,
                'value_scaling': scaling_formula,
                'duration': duration,
                'tick_type': tick_type
            }
            
            result = save_effect(new_effect)
            if result:
                st.session_state.creating_new_effect = False
                st.success("Effect created successfully!")
                st.rerun()
            else:
                st.error("Failed to create effect. Please check all fields are filled correctly.")