# ./ClassManager/JobClassEditor/stats_tab.py

import streamlit as st
from typing import Dict, Any

def render_stats_tab(current_record: Dict[str, Any]) -> Dict[str, Any]:
    """Render the Stats tab and return its data"""
    st.subheader("Starting Stats (Level 1)")
    col1, col2, col3 = st.columns(3)
    with col1:
        base_hp = st.number_input("Base HP", value=current_record.get('base_hp', 0), key="base_hp_input")
        base_mp = st.number_input("Base MP", value=current_record.get('base_mp', 0), key="base_mp_input")
        base_physical_attack = st.number_input("Base Physical Attack", value=current_record.get('base_physical_attack', 0), key="base_physical_attack_input")
    with col2:
        base_physical_defense = st.number_input("Base Physical Defense", value=current_record.get('base_physical_defense', 0), key="base_physical_defense_input")
        base_agility = st.number_input("Base Agility", value=current_record.get('base_agility', 0), key="base_agility_input")
        base_magical_attack = st.number_input("Base Magical Attack", value=current_record.get('base_magical_attack', 0), key="base_magical_attack_input")
    with col3:
        base_magical_defense = st.number_input("Base Magical Defense", value=current_record.get('base_magical_defense', 0), key="base_magical_defense_input")
        base_resistance = st.number_input("Base Resistance", value=current_record.get('base_resistance', 0), key="base_resistance_input")
        base_special = st.number_input("Base Special", value=current_record.get('base_special', 0), key="base_special_input")

    st.subheader("Stats Per Level (Beyond Level 1)")
    col1, col2, col3 = st.columns(3)
    with col1:
        hp_per_level = st.number_input("HP per Level", value=current_record.get('hp_per_level', 0), key="hp_per_level_input")
        mp_per_level = st.number_input("MP per Level", value=current_record.get('mp_per_level', 0), key="mp_per_level_input")
        physical_attack_per_level = st.number_input("Physical Attack per Level", value=current_record.get('physical_attack_per_level', 0), key="physical_attack_per_level_input")
    with col2:
        physical_defense_per_level = st.number_input("Physical Defense per Level", value=current_record.get('physical_defense_per_level', 0), key="physical_defense_per_level_input")
        agility_per_level = st.number_input("Agility per Level", value=current_record.get('agility_per_level', 0), key="agility_per_level_input")
        magical_attack_per_level = st.number_input("Magical Attack per Level", value=current_record.get('magical_attack_per_level', 0), key="magical_attack_per_level_input")
    with col3:
        magical_defense_per_level = st.number_input("Magical Defense per Level", value=current_record.get('magical_defense_per_level', 0), key="magical_defense_per_level_input")
        resistance_per_level = st.number_input("Resistance per Level", value=current_record.get('resistance_per_level', 0), key="resistance_per_level_input")
        special_per_level = st.number_input("Special per Level", value=current_record.get('special_per_level', 0), key="special_per_level_input")

    return {
        'base_hp': base_hp,
        'base_mp': base_mp,
        'base_physical_attack': base_physical_attack,
        'base_physical_defense': base_physical_defense,
        'base_agility': base_agility,
        'base_magical_attack': base_magical_attack,
        'base_magical_defense': base_magical_defense,
        'base_resistance': base_resistance,
        'base_special': base_special,
        'hp_per_level': hp_per_level,
        'mp_per_level': mp_per_level,
        'physical_attack_per_level': physical_attack_per_level,
        'physical_defense_per_level': physical_defense_per_level,
        'agility_per_level': agility_per_level,
        'magical_attack_per_level': magical_attack_per_level,
        'magical_defense_per_level': magical_defense_per_level,
        'resistance_per_level': resistance_per_level,
        'special_per_level': special_per_level
    }