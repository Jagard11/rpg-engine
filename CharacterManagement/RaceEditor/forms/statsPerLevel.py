# ./CharacterManagement/RaceEditor/forms/statsPerLevel.py

import streamlit as st
from typing import Dict, Optional

def render_stats_per_level_tab(race_data: Optional[Dict] = None) -> Dict:
    """Render the stats per level tab"""
    col1, col2, col3 = st.columns(3)
    
    with col1:
        hp_per_level = st.number_input(
            "HP per Level",
            value=float(race_data.get('hp_per_level', 0)) if race_data else 0.0
        )
        mp_per_level = st.number_input(
            "MP per Level",
            value=float(race_data.get('mp_per_level', 0)) if race_data else 0.0
        )
        physical_attack_per_level = st.number_input(
            "Physical Attack per Level",
            value=float(race_data.get('physical_attack_per_level', 0)) if race_data else 0.0
        )
    
    with col2:
        physical_defense_per_level = st.number_input(
            "Physical Defense per Level",
            value=float(race_data.get('physical_defense_per_level', 0)) if race_data else 0.0
        )
        magical_attack_per_level = st.number_input(
            "Magical Attack per Level",
            value=float(race_data.get('magical_attack_per_level', 0)) if race_data else 0.0
        )
        magical_defense_per_level = st.number_input(
            "Magical Defense per Level",
            value=float(race_data.get('magical_defense_per_level', 0)) if race_data else 0.0
        )
    
    with col3:
        agility_per_level = st.number_input(
            "Agility per Level",
            value=float(race_data.get('agility_per_level', 0)) if race_data else 0.0
        )
        resistance_per_level = st.number_input(
            "Resistance per Level",
            value=float(race_data.get('resistance_per_level', 0)) if race_data else 0.0
        )
        special_per_level = st.number_input(
            "Special per Level",
            value=float(race_data.get('special_per_level', 0)) if race_data else 0.0
        )
    
    return {
        'hp_per_level': hp_per_level,
        'mp_per_level': mp_per_level,
        'physical_attack_per_level': physical_attack_per_level,
        'physical_defense_per_level': physical_defense_per_level,
        'magical_attack_per_level': magical_attack_per_level,
        'magical_defense_per_level': magical_defense_per_level,
        'agility_per_level': agility_per_level,
        'resistance_per_level': resistance_per_level,
        'special_per_level': special_per_level
    }