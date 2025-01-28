# ./SpellManager/spellEditor.py

import streamlit as st
from .database import (
    load_spell_tiers, 
    load_spell_type, 
    save_spell,
    load_spell_requirements,
    save_spell_requirements,
    load_spell_procedures,
    save_spell_procedures
)
from .spellEffectsEditor import render_spell_effects_section

def render_requirements_section(spell_id: int):
    """Render the requirements section for a spell"""
    st.subheader("Spell Requirements")
    
    # Load existing requirements
    requirements = load_spell_requirements(spell_id)
    
    # Group requirements
    requirement_groups = {}
    for req in requirements:
        group = req['requirement_group']
        if group not in requirement_groups:
            requirement_groups[group] = []
        requirement_groups[group].append(req)
    
    # Display existing requirement groups
    for group_num, group_reqs in requirement_groups.items():
        with st.expander(f"Requirement Group {group_num}"):
            st.write("All these conditions must be met:")
            for req in group_reqs:
                col1, col2, col3, col4 = st.columns([2, 2, 2, 1])
                with col1:
                    st.text(req['requirement_type'])
                with col2:
                    st.text(f"Target: {req['target_type']}")
                with col3:
                    st.text(f"{req['target_value']}")
                with col4:
                    if st.button(f"Delete {req['id']}"):
                        # Handle deletion
                        requirements = [r for r in requirements if r['id'] != req['id']]
                        save_spell_requirements(spell_id, requirements)
                        st.rerun()
    
    # Add new requirement
    if st.button("Add Requirement Group"):
        new_group = max(requirement_groups.keys()) + 1 if requirement_groups else 1
        st.session_state.adding_requirement_group = new_group
        
    if st.session_state.get('adding_requirement_group'):
        with st.form("new_requirement_form"):
            st.write(f"Adding requirements for group {st.session_state.adding_requirement_group}")
            
            requirement_type = st.selectbox(
                "Requirement Type",
                ['status_effect', 'range', 'target_status', 'resource', 'spell_state']
            )
            
            target_type = st.selectbox(
                "Target Type",
                ['self', 'target', 'area']
            )
            
            target_value = st.text_input("Target Value")
            
            comparison_type = st.selectbox(
                "Comparison Type",
                ['equals', 'greater_than', 'less_than', 'has_status', 'in_range']
            )
            
            value = st.number_input("Value", value=0) if comparison_type != 'has_status' else None
            
            if st.form_submit_button("Add Requirement"):
                new_req = {
                    'spell_id': spell_id,
                    'requirement_group': st.session_state.adding_requirement_group,
                    'requirement_type': requirement_type,
                    'target_type': target_type,
                    'target_value': target_value,
                    'comparison_type': comparison_type,
                    'value': value
                }
                requirements.append(new_req)
                save_spell_requirements(spell_id, requirements)
                st.session_state.adding_requirement_group = None
                st.rerun()

def render_procedures_section(spell_id: int):
    """Render the procedures section for a spell"""
    st.subheader("Spell Procedures")
    
    # Load existing procedures
    procedures = load_spell_procedures(spell_id)
    
    # Group procedures by trigger type
    trigger_groups = {}
    for proc in procedures:
        trigger = proc['trigger_type']
        if trigger not in trigger_groups:
            trigger_groups[trigger] = []
        trigger_groups[trigger].append(proc)
    
    # Display existing procedures
    for trigger, procs in trigger_groups.items():
        with st.expander(f"{trigger.replace('_', ' ').title()} Procedures"):
            for proc in sorted(procs, key=lambda x: x['proc_order']):
                col1, col2, col3, col4 = st.columns([2, 2, 2, 1])
                with col1:
                    st.text(proc['action_type'])
                with col2:
                    st.text(f"Target: {proc['target_type']}")
                with col3:
                    st.text(f"{proc['action_value']}")
                with col4:
                    if st.button(f"Delete {proc['id']}"):
                        procedures = [p for p in procedures if p['id'] != proc['id']]
                        save_spell_procedures(spell_id, procedures)
                        st.rerun()
    
    # Add new procedure
    if st.button("Add Procedure"):
        st.session_state.adding_procedure = True
        
    if st.session_state.get('adding_procedure'):
        with st.form("new_procedure_form"):
            trigger_type = st.selectbox(
                "Trigger Type",
                ['on_cast', 'on_hit', 'on_crit', 'on_kill']
            )
            
            action_type = st.selectbox(
                "Action Type",
                ['apply_state', 'remove_state', 'cast_spell', 'modify_resource']
            )
            
            target_type = st.selectbox(
                "Target Type",
                ['self', 'target', 'area']
            )
            
            action_value = st.text_input("Action Value")
            
            value_modifier = st.number_input("Value Modifier", value=1)
            
            chance = st.slider(
                "Chance",
                min_value=0.0,
                max_value=1.0,
                value=1.0,
                step=0.1
            )
            
            # Calculate next proc order for this trigger
            same_trigger_procs = [p for p in procedures if p['trigger_type'] == trigger_type]
            proc_order = max([p['proc_order'] for p in same_trigger_procs], default=0) + 1
            
            if st.form_submit_button("Add Procedure"):
                new_proc = {
                    'spell_id': spell_id,
                    'trigger_type': trigger_type,
                    'proc_order': proc_order,
                    'action_type': action_type,
                    'target_type': target_type,
                    'action_value': action_value,
                    'value_modifier': value_modifier,
                    'chance': chance
                }
                procedures.append(new_proc)
                save_spell_procedures(spell_id, procedures)
                st.session_state.adding_procedure = False
                st.rerun()

def render_spell_editor():
    """Interface for editing individual spells"""
    spell_data = st.session_state.get('editing_spell', {})
    
    with st.form("spell_base_editor_form"):
        # Basic spell info section
        st.subheader("Basic Information")
        col_id, col_name = st.columns([1, 4])
        
        with col_id:
            spell_id = st.text_input(
                "Record ID",
                value=str(spell_data.get('id', '0')),
                disabled=True
            )
        
        with col_name:
            name = st.text_input("Spell Name", value=spell_data.get('name', ''))
        
        # Existing spell editor fields...
        
        # Save button
        submitted = st.form_submit_button("Save Spell")
        if submitted:
            if save_spell(spell_data):
                st.success("Spell saved successfully!")
                st.session_state.editing_spell = spell_data
                st.rerun()
    
    # Requirements and Procedures sections (outside the main form)
    if spell_data.get('id'):
        st.markdown("---")
        render_requirements_section(spell_data['id'])
        
        st.markdown("---")
        render_procedures_section(spell_data['id'])
        
        st.markdown("---")
        render_spell_effects_section(spell_data['id'])
    else:
        st.info("Save the spell first to add requirements, procedures, and effects")