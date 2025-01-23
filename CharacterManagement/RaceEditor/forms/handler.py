# ./CharacterManagement/RaceEditor/forms/handler.py

import streamlit as st
from typing import Dict, Optional
import sqlite3

from .baseInfo import render_basic_info_tab
from .baseStats import render_base_stats_tab
from .statsPerLevel import render_stats_per_level_tab
from .prerequisites import render_prerequisites_tab

def save_race_data(race_data: Dict) -> tuple[bool, str]:
    """Save race data to database"""
    try:
        conn = sqlite3.connect('rpg_data.db')
        cursor = conn.cursor()
        
        if race_data.get('id'):
            # Update existing race
            fields = [f"{k} = ?" for k in race_data.keys() if k != 'id']
            query = f"""
                UPDATE races 
                SET {', '.join(fields)}
                WHERE id = ?
            """
            values = [v for k, v in race_data.items() if k != 'id']
            values.append(race_data['id'])
            
            cursor.execute(query, values)
        else:
            # Insert new race
            fields = [k for k in race_data.keys() if k != 'id']
            placeholders = ['?' for _ in fields]
            query = f"""
                INSERT INTO races ({', '.join(fields)})
                VALUES ({', '.join(placeholders)})
            """
            values = [race_data[k] for k in fields]
            
            cursor.execute(query, values)
            race_data['id'] = cursor.lastrowid
        
        conn.commit()
        return True, f"Race {'updated' if race_data.get('id') else 'created'} successfully!"
        
    except Exception as e:
        return False, f"Error saving race: {str(e)}"
    finally:
        conn.close()

def render_race_form(race_data: Optional[Dict] = None) -> None:
    """Render the complete race form with tabs"""
    
    # Create tabs
    tab1, tab2, tab3, tab4 = st.tabs([
        "Basic Information",
        "Base Stats",
        "Stats per Level",
        "Prerequisites"
    ])
    
    # Render each tab
    with tab1:
        basic_info = render_basic_info_tab(race_data)
    
    with tab2:
        base_stats = render_base_stats_tab(race_data)
    
    with tab3:
        stats_per_level = render_stats_per_level_tab(race_data)
    
    with tab4:
        prerequisites = render_prerequisites_tab(race_data)
    
    # Main save button for race data
    if st.button("Save Race"):
        if not basic_info.get('name'):
            st.error("Name is required!")
            return
            
        # Combine all data
        save_data = {
            'id': race_data.get('id') if race_data else None,
            **basic_info,
            **base_stats,
            **stats_per_level
        }
        
        success, message = save_race_data(save_data)
        if success:
            st.success(message)
            if not race_data:  # If this was a new race
                st.rerun()  # Refresh to show prerequisites tab
        else:
            st.error(message)