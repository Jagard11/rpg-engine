# ./CharacterManagement/RaceEditor/forms/baseStats.py

import streamlit as st
from typing import Dict, Optional

def render_base_stats_tab(race_data: Optional[Dict] = None) -> Dict:
    """Render the base stats tab"""
    col1, col2, col3 = st.columns(3)
    
    with col1:
        base_hp = st.number_input(
            "Base HP",
            value=float(race_data.get('base_hp', 0)) if race_data else 0.0
        )
        base_mp = st.number_input(
            "Base MP",
            value=float(race_data.get('base_mp', 0)) if race_data else 0.0
        )
        base_physical_attack = st.number_input(
            "Base Physical Attack",
            value=float(race_data.get('base_physical_attack', 0)) if race_data else 0.0
        )
    
    with col2:
        base_physical_defense = st.number_input(
            "Base Physical Defense",
            value=float(race_data.get('base_physical_defense', 0)) if race_data else 0.0
        )
        base_magical_attack = st.number_input(
            "Base Magical Attack",
            value=float(race_data.get('base_magical_attack', 0)) if race_data else 0.0
        )
        base_magical_defense = st.number_input(
            "Base Magical Defense",
            value=float(race_data.get('base_magical_defense', 0)) if race_data else 0.0
        )
    
    with col3:
        base_agility = st.number_input(
            "Base Agility",
            value=float(race_data.get('base_agility', 0)) if race_data else 0.0
        )
        base_resistance = st.number_input(
            "Base Resistance",
            value=float(race_data.get('base_resistance', 0)) if race_data else 0.0
        )
        base_special = st.number_input(
            "Base Special",
            value=float(race_data.get('base_special', 0)) if race_data else 0.0
        )
    
    return {
        'base_hp': base_hp,
        'base_mp': base_mp,
        'base_physical_attack': base_physical_attack,
        'base_physical_defense': base_physical_defense,
        'base_magical_attack': base_magical_attack,
        'base_magical_defense': base_magical_defense,
        'base_agility': base_agility,
        'base_resistance': base_resistance,
        'base_special': base_special
    }