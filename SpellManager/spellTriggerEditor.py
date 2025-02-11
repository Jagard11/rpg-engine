# ./SpellManager/spellTriggerEditor.py

import streamlit as st
from typing import Dict, Optional
from .database import (
    load_spell_activation_types,
    save_spell_activation_requirement,
    load_spell_activation_requirements
)

def render_trigger_editor(spell_id: Optional[int] = None):
    """Main interface for spell trigger configuration"""
    if not spell_id:
        st.info("Please save the spell first to configure triggers")
        return

    st.header("Trigger Configuration")
    
    # Load activation types
    activation_types = load_spell_activation_types()
    
    # Create tabs for different trigger aspects
    trigger_tab, conditions_tab, resources_tab = st.tabs([
        "Trigger Type", 
        "Conditions", 
        "Resource Management"
    ])
    
    with trigger_tab:
        render_trigger_type_section(spell_id, activation_types)
        
    with conditions_tab:
        render_conditions_section(spell_id)
        
    with resources_tab:
        render_resource_section(spell_id)

def render_trigger_type_section(spell_id: int, activation_types: list):
    """Render trigger type selection and configuration"""
    st.subheader("Trigger Type")
    
    with st.form("trigger_type_form"):
        activation_type = st.selectbox(
            "Activation Method",
            options=[t['id'] for t in activation_types],
            format_func=lambda x: next(t['name'] for t in activation_types if t['id'] == x),
            key="activation_type"
        )
        
        col1, col2 = st.columns(2)
        with col1:
            is_passive = st.checkbox(
                "Passive Effect",
                help="Effect is always active when conditions are met"
            )
            
        with col2:
            requires_activation = st.checkbox(
                "Requires Activation",
                help="Must be manually activated when conditions are met"
            )
            
        priority = st.slider(
            "Trigger Priority",
            min_value=1,
            max_value=10,
            value=5,
            help="Higher priority triggers are processed first"
        )
        
        if st.form_submit_button("Save Trigger Type"):
            # Save trigger type configuration
            pass

def render_conditions_section(spell_id: int):
    """Render trigger conditions configuration"""
    st.subheader("Trigger Conditions")
    
    # Load existing conditions
    requirements = load_spell_activation_requirements(spell_id)
    
    # Display existing conditions
    if requirements:
        for req in requirements:
            with st.expander(f"Condition: {req['requirement_type']}"):
                st.write(f"Type: {req['requirement_type']}")
                st.write(f"Value: {req['requirement_value']}")
                if st.button("Delete Condition", key=f"del_cond_{req['id']}"):
                    # Delete condition logic
                    pass
    
    # Add new condition
    with st.form("new_condition_form"):
        requirement_type = st.selectbox(
            "Condition Type",
            options=[
                "class_requirement",
                "race_requirement",
                "stat_requirement",
                "state_requirement",
                "item_requirement",
                "quest_requirement"
            ]
        )
        
        requirement_value = st.text_input("Condition Value")
        
        if st.form_submit_button("Add Condition"):
            new_req = {
                'spell_id': spell_id,
                'requirement_type': requirement_type,
                'requirement_value': requirement_value
            }
            if save_spell_activation_requirement(new_req):
                st.success("Condition added successfully!")
                st.rerun()

def render_resource_section(spell_id: int):
    """Render resource management configuration"""
    st.subheader("Resource Management")
    
    with st.form("resource_management_form"):
        col1, col2 = st.columns(2)
        
        with col1:
            mp_cost = st.number_input(
                "MP Cost",
                min_value=0,
                help="Mana cost to activate"
            )
            
            cooldown = st.number_input(
                "Cooldown (seconds)",
                min_value=0.0,
                help="Time before trigger can activate again"
            )
        
        with col2:
            charges = st.number_input(
                "Maximum Charges",
                min_value=1,
                help="Number of times trigger can activate before recharge"
            )
            
            recharge_time = st.number_input(
                "Recharge Time (seconds)",
                min_value=0.0,
                help="Time to restore one charge"
            )
        
        shared_cooldown_group = st.text_input(
            "Shared Cooldown Group",
            help="Triggers in same group share cooldown"
        )
        
        if st.form_submit_button("Save Resource Configuration"):
            # Save resource configuration
            pass