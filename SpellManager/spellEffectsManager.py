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
    
    # Load available effects
    available_effects = load_effects()
    current_effects = load_spell_effects(spell_id)
    
    # Filter out already selected effects
    if current_effects:
        current_effect_ids = {e['id'] for e in current_effects}
        available_effects = [e for e in available_effects if e['id'] not in current_effect_ids]
    
    # Display current effects
    if current_effects:
        st.write("Current Effects:")
        for effect in current_effects:
            with st.expander(f"{effect['name']} ({effect['type_name']})"):
                st.write(f"Base Value: {effect['base_value']}")
                st.write(f"Scaling: {effect['value_scaling']}")
                if st.button("Remove Effect", key=f"remove_effect_{effect['id']}"):
                    current_effects.remove(effect)
                    save_spell_effects(spell_id, current_effects)
                    st.rerun()
    
    # Add existing effect
    if available_effects:
        with st.form("add_effect_form"):
            st.write("Add Existing Effect:")
            effect_options = {str(e['id']): f"{e['name']} ({e['type_name']})" 
                            for e in available_effects}
            
            selected_effect = st.selectbox(
                "Select Effect",
                options=list(effect_options.keys()),
                format_func=lambda x: effect_options[x]
            )
            
            if st.form_submit_button("Add Effect"):
                effect = next(e for e in available_effects 
                            if str(e['id']) == selected_effect)
                if current_effects is None:
                    current_effects = []
                current_effects.append(effect)
                save_spell_effects(spell_id, current_effects)
                st.rerun()
    
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
    
    for effect in current_effects:
        with st.expander(f"Configure {effect['name']}"):
            with st.form(f"effect_params_{effect['id']}"):
                col1, col2 = st.columns(2)
                
                with col1:
                    base_value = st.number_input(
                        "Base Value",
                        value=effect['base_value']
                    )
                    
                    scaling_formula = st.text_input(
                        "Scaling Formula",
                        value=effect['value_scaling']
                    )
                
                with col2:
                    duration = st.number_input(
                        "Duration (seconds)",
                        min_value=0,
                        value=effect.get('duration', 0)
                    )
                    
                    tick_rate = st.number_input(
                        "Tick Rate (seconds)",
                        min_value=0.0,
                        value=effect.get('tick_rate', 1.0)
                    )
                
                if st.form_submit_button("Update Parameters"):
                    effect.update({
                        'base_value': base_value,
                        'value_scaling': scaling_formula,
                        'duration': duration,
                        'tick_rate': tick_rate
                    })
                    save_spell_effects(spell_id, current_effects)
                    st.success("Parameters updated!")

def render_effect_chaining(spell_id: int):
    """Interface for chaining multiple effects"""
    st.subheader("Effect Chain Configuration")
    
    current_effects = load_spell_effects(spell_id)
    if not current_effects:
        st.info("Add effects first to configure chaining")
        return
    
    st.write("Drag effects to reorder the chain:")
    
    # Display effects in order with up/down buttons
    for i, effect in enumerate(current_effects):
        col1, col2, col3 = st.columns([3, 1, 1])
        
        with col1:
            st.write(f"{i+1}. {effect['name']}")
        
        with col2:
            if i > 0 and st.button("↑", key=f"move_up_{effect['id']}"):
                current_effects[i], current_effects[i-1] = current_effects[i-1], current_effects[i]
                save_spell_effects(spell_id, current_effects)
                st.rerun()
        
        with col3:
            if i < len(current_effects)-1 and st.button("↓", key=f"move_down_{effect['id']}"):
                current_effects[i], current_effects[i+1] = current_effects[i+1], current_effects[i]
                save_spell_effects(spell_id, current_effects)
                st.rerun()
    
    # Chain conditions
    st.write("---")
    st.write("Chain Conditions")
    
    with st.form("chain_conditions"):
        require_all_effects = st.checkbox(
            "Require All Effects",
            help="All effects must successfully apply for any to take effect"
        )
        
        stop_on_failure = st.checkbox(
            "Stop on Failure",
            help="Stop applying effects if one fails"
        )
        
        if st.form_submit_button("Save Chain Settings"):
            # Save chain settings
            pass

def render_new_effect_form(spell_id: int):
    """Form for creating a new effect"""
    effect_types = load_effect_types()
    
    with st.form("new_effect_form"):
        st.write("Create New Effect")
        
        name = st.text_input("Effect Name")
        
        # Use the actual effect type IDs from the database
        type_options = {str(t['id']): t['name'] for t in effect_types}
        selected_type = st.selectbox(
            "Effect Type",
            options=list(type_options.keys()),
            format_func=lambda x: type_options[x]
        )
        
        description = st.text_area("Description")
        
        col1, col2 = st.columns(2)
        
        with col1:
            base_value = st.number_input("Base Value")
            duration = st.number_input("Duration (seconds)", min_value=0)
        
        with col2:
            scaling_formula = st.text_input("Scaling Formula")
            tick_rate = st.number_input("Tick Rate (seconds)", min_value=0.0, value=1.0)
        
        if st.form_submit_button("Create Effect"):
            new_effect = {
                'name': name,
                'effect_type_id': int(selected_type),  # Convert string ID back to integer
                'description': description,
                'base_value': base_value,
                'value_scaling': scaling_formula,
                'duration': duration,
                'tick_type': 'immediate',  # Default value
                'tick_rate': tick_rate
            }
            
            if save_effect(new_effect):
                st.session_state.creating_new_effect = False
                st.success("Effect created successfully!")
                st.rerun()
            else:
                st.error("Failed to create effect. Please check all fields are filled correctly.")