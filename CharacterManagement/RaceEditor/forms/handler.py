# ./CharacterManagement/RaceEditor/forms/handler.py

import streamlit as st
from typing import Dict, Optional
from .baseInfo import render_basic_info_tab, get_class_types, get_race_categories, get_subcategories
from .baseStats import render_base_stats_tab
from .statsPerLevel import render_stats_per_level_tab
from .prerequisites import render_prerequisites_tab
from .raceSelect import render_race_select_tab

def render_race_form(race_data: Optional[Dict] = None) -> Dict:
    """Render the race editor form with tabs"""
    
    # Get filter options
    class_types = get_class_types()
    categories = get_race_categories()
    subcategories = get_subcategories()
    
    tabs = st.tabs([
        "Race Select",
        "Basic Information",
        "Base Stats",
        "Stats Per Level",
        "Prerequisites"
    ])
    
    # Render each tab
        
    with tabs[0]:
        render_race_select_tab(class_types, categories, subcategories)
        
    with tabs[1]:
        basic_info = render_basic_info_tab(race_data)
            
    with tabs[2]:
        base_stats = render_base_stats_tab(race_data)
            
    with tabs[3]:
        stats_per_level = render_stats_per_level_tab(race_data)
            
    with tabs[4]:
        prerequisites = render_prerequisites_tab(race_data)
    
    # Submit button
    submitted = st.button("Save Race")
    
    if submitted:
        if not basic_info.get('name'):
            st.error("Name is required!")
            return None
            
        return {
            'id': race_data.get('id') if race_data else None,
            **basic_info,
            **base_stats,
            **stats_per_level,
            **prerequisites
        }
    
    return None