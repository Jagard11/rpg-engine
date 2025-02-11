# ./SpellManager/spellTargetingEditor.py

import streamlit as st
from typing import Dict, Optional
from .database import (
    save_spell_targeting,
    load_spell_targeting
)

def render_targeting_editor(spell_id: Optional[int] = None):
    """Main interface for spell targeting configuration"""
    if not spell_id:
        st.info("Please save the spell first to configure targeting")
        return

    st.header("Targeting System")
    
    # Create tabs for different targeting aspects
    target_tab, range_tab, area_tab = st.tabs([
        "Target Selection",
        "Range Configuration",
        "Area Definition"
    ])
    
    with target_tab:
        render_target_selection(spell_id)
        
    with range_tab:
        render_range_configuration(spell_id)
        
    with area_tab:
        render_area_definition(spell_id)

def render_target_selection(spell_id: int):
    """Interface for configuring target selection"""
    st.subheader("Target Selection")
    
    targeting_data = load_spell_targeting(spell_id)
    
    with st.form("target_selection_form"):
        target_type = st.selectbox(
            "Target Type",
            options=[
                "single_target",
                "multi_target",
                "self",
                "ally",
                "enemy",
                "area",
                "ground"
            ],
            index=0 if not targeting_data else 
                  ["single_target", "multi_target", "self", "ally", "enemy", "area", "ground"].index(targeting_data.get('target_type', 'single_target'))
        )
        
        col1, col2 = st.columns(2)
        with col1:
            max_targets = st.number_input(
                "Maximum Targets",
                min_value=1,
                value=targeting_data.get('max_targets', 1),
                disabled=target_type in ['self', 'area', 'ground']
            )
            
            requires_los = st.checkbox(
                "Requires Line of Sight",
                value=targeting_data.get('requires_los', True)
            )
        
        with col2:
            allow_dead_targets = st.checkbox(
                "Allow Dead Targets",
                value=targeting_data.get('allow_dead_targets', False)
            )
            
            ignore_target_immunity = st.checkbox(
                "Ignore Target Immunity",
                value=targeting_data.get('ignore_target_immunity', False)
            )
        
        targeting_restrictions = st.multiselect(
            "Targeting Restrictions",
            options=[
                "living_only",
                "undead_only",
                "humanoid_only",
                "beast_only",
                "construct_only"
            ],
            default=targeting_data.get('targeting_restrictions', [])
        )
        
        if st.form_submit_button("Save Target Configuration"):
            targeting_config = {
                'spell_id': spell_id,
                'target_type': target_type,
                'max_targets': max_targets,
                'requires_los': requires_los,
                'allow_dead_targets': allow_dead_targets,
                'ignore_target_immunity': ignore_target_immunity,
                'targeting_restrictions': targeting_restrictions
            }
            if save_spell_targeting(targeting_config):
                st.success("Target configuration saved!")

def render_range_configuration(spell_id: int):
    """Interface for configuring spell range"""
    st.subheader("Range Configuration")
    
    targeting_data = load_spell_targeting(spell_id)
    
    with st.form("range_config_form"):
        col1, col2 = st.columns(2)
        
        with col1:
            min_range = st.number_input(
                "Minimum Range (units)",
                min_value=0,
                value=targeting_data.get('min_range', 0)
            )
            
            max_range = st.number_input(
                "Maximum Range (units)",
                min_value=min_range,
                value=targeting_data.get('max_range', 30)
            )
        
        with col2:
            vertical_range = st.number_input(
                "Vertical Range (units)",
                min_value=0,
                value=targeting_data.get('vertical_range', 10)
            )
            
            range_type = st.selectbox(
                "Range Type",
                options=["cylinder", "sphere", "cone"],
                index=["cylinder", "sphere", "cone"].index(targeting_data.get('range_type', 'cylinder'))
            )
        
        st.write("Range Modifiers")
        col1, col2 = st.columns(2)
        
        with col1:
            weather_modifier = st.number_input(
                "Weather Modifier",
                min_value=0.0,
                max_value=2.0,
                value=targeting_data.get('weather_modifier', 1.0),
                help="Range multiplier during weather effects"
            )
        
        with col2:
            terrain_modifier = st.number_input(
                "Terrain Modifier",
                min_value=0.0,
                max_value=2.0,
                value=targeting_data.get('terrain_modifier', 1.0),
                help="Range multiplier based on terrain"
            )
        
        if st.form_submit_button("Save Range Configuration"):
            range_config = {
                'spell_id': spell_id,
                'min_range': min_range,
                'max_range': max_range,
                'vertical_range': vertical_range,
                'range_type': range_type,
                'weather_modifier': weather_modifier,
                'terrain_modifier': terrain_modifier
            }
            if save_spell_targeting(range_config):
                st.success("Range configuration saved!")

def render_area_definition(spell_id: int):
    """Interface for configuring area of effect"""
    st.subheader("Area Definition")
    
    targeting_data = load_spell_targeting(spell_id)
    
    with st.form("area_definition_form"):
        area_type = st.selectbox(
            "Area Type",
            options=["circle", "square", "cone", "line"],
            index=["circle", "square", "cone", "line"].index(targeting_data.get('area_type', 'circle'))
        )
        
        col1, col2 = st.columns(2)
        
        with col1:
            area_size = st.number_input(
                "Area Size (units)",
                min_value=1,
                value=targeting_data.get('area_size', 10)
            )
            
            if area_type == "cone":
                cone_angle = st.number_input(
                    "Cone Angle (degrees)",
                    min_value=1,
                    max_value=360,
                    value=targeting_data.get('cone_angle', 90)
                )
        
        with col2:
            area_duration = st.number_input(
                "Area Duration (seconds)",
                min_value=0,
                value=targeting_data.get('area_duration', 0),
                help="0 for instant effect"
            )
            
            if area_type == "line":
                line_width = st.number_input(
                    "Line Width (units)",
                    min_value=1,
                    value=targeting_data.get('line_width', 1)
                )
        
        st.write("Area Effects")
        persist_after_cast = st.checkbox(
            "Persist After Cast",
            value=targeting_data.get('persist_after_cast', False),
            help="Area effect remains after spell completes"
        )
        
        if persist_after_cast:
            col1, col2 = st.columns(2)
            with col1:
                tick_rate = st.number_input(
                    "Effect Tick Rate (seconds)",
                    min_value=0.1,
                    value=targeting_data.get('tick_rate', 1.0)
                )
            
            with col2:
                max_ticks = st.number_input(
                    "Maximum Ticks",
                    min_value=1,
                    value=targeting_data.get('max_ticks', 5)
                )
        
        if st.form_submit_button("Save Area Configuration"):
            area_config = {
                'spell_id': spell_id,
                'area_type': area_type,
                'area_size': area_size,
                'area_duration': area_duration,
                'persist_after_cast': persist_after_cast
            }
            
            if area_type == "cone":
                area_config['cone_angle'] = cone_angle
            elif area_type == "line":
                area_config['line_width'] = line_width
                
            if persist_after_cast:
                area_config.update({
                    'tick_rate': tick_rate,
                    'max_ticks': max_ticks
                })
                
            if save_spell_targeting(area_config):
                st.success("Area configuration saved!")