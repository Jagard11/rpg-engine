# ./CharacterInfo.py

import streamlit as st
import sqlite3
from dataclasses import dataclass
from typing import List, Optional, Dict
from datetime import datetime
import json

@dataclass
class Character:
    """Core character data"""
    id: int
    name: str
    race_id: Optional[int]
    character_level: int
    experience: int
    karma: int
    current_health: int
    max_health: int
    current_mana: int
    max_mana: int
    description: Optional[str]
    is_active: bool
    created_at: str
    updated_at: str

def get_db_connection():
    """Create a database connection"""
    return sqlite3.connect('rpg_data.db')

def init_character_state():
    """Initialize character-related session state variables"""
    if 'current_character' not in st.session_state:
        st.session_state.current_character = None
    if 'character_list' not in st.session_state:
        st.session_state.character_list = []
        load_character_list()

def load_character_list():
    """Load list of available characters from database"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT c.id, c.name, r.name as race, c.character_level 
            FROM characters c
            LEFT JOIN races r ON c.race_id = r.id
            ORDER BY c.name
        """)
        st.session_state.character_list = cursor.fetchall()
    except Exception as e:
        st.error(f"Error loading character list: {str(e)}")
    finally:
        conn.close()

def load_character(character_id: int) -> Optional[Character]:
    """Load a character's basic information"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT * FROM characters WHERE id = ?
        """, (character_id,))
        result = cursor.fetchone()
        if result:
            # Convert tuple to dict using column names
            columns = [description[0] for description in cursor.description]
            char_dict = dict(zip(columns, result))
            return Character(**char_dict)
        return None
    except Exception as e:
        st.error(f"Error loading character: {str(e)}")
        return None
    finally:
        conn.close()

def load_character_stats(character_id: int) -> List[Dict]:
    """Load character stats with current values"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT cs.*, bs.name as stat_name 
            FROM character_stats cs
            JOIN base_stats bs ON cs.stat_id = bs.id
            WHERE cs.character_id = ?
        """, (character_id,))
        columns = [description[0] for description in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except Exception as e:
        st.error(f"Error loading character stats: {str(e)}")
        return []
    finally:
        conn.close()

def load_character_class_info(character_id: int) -> List[Dict]:
    """Load character class progression information"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                ccp.*,
                cc.name as class_name,
                cc.type as class_type
            FROM character_class_progression ccp
            JOIN character_classes cc ON ccp.class_id = cc.id
            WHERE ccp.character_id = ?
            ORDER BY cc.type, cc.name
        """, (character_id,))
        columns = [description[0] for description in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except Exception as e:
        st.error(f"Error loading class information: {str(e)}")
        return []
    finally:
        conn.close()

def load_active_abilities(character_id: int) -> List[Dict]:
    """Load character's currently available abilities"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT 
                ca.*,
                a.name as ability_name,
                a.description,
                a.type,
                a.cooldown,
                a.mana_cost,
                a.damage,
                a.healing
            FROM character_abilities ca
            JOIN abilities a ON ca.ability_id = a.id
            WHERE ca.character_id = ?
            ORDER BY a.type, a.name
        """, (character_id,))
        columns = [description[0] for description in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except Exception as e:
        st.error(f"Error loading abilities: {str(e)}")
        return []
    finally:
        conn.close()

def render_character_tab():
    """Display character information"""
    st.header("Character Information")
    
    # Character selection
    if st.session_state.character_list:
        # Format display string for each character
        char_options = [
            f"{char[1]} (Level {char[3]} {char[2] or 'Unknown Race'})" 
            for char in st.session_state.character_list
        ]
        selected_index = st.selectbox(
            "Select Character",
            range(len(char_options)),
            format_func=lambda x: char_options[x],
            key="char_select"
        )
        
        if selected_index is not None:
            character_id = st.session_state.character_list[selected_index][0]
            character = load_character(character_id)
            
            if character:
                st.session_state.current_character = character
                
                # Basic Info and Stats in two columns
                col1, col2 = st.columns(2)
                
                with col1:
                    st.subheader("Basic Information")
                    st.write(f"Name: {character.name}")
                    st.write(f"Level: {character.character_level}")
                    st.write(f"Experience: {character.experience}")
                    st.write(f"Karma: {character.karma}")
                    st.write(f"Health: {character.current_health}/{character.max_health}")
                    st.write(f"Mana: {character.current_mana}/{character.max_mana}")
                
                with col2:
                    st.subheader("Stats")
                    stats = load_character_stats(character.id)
                    for stat in stats:
                        total = (stat['base_value'] + stat['bonus_racial'] + 
                                stat['bonus_class'] + stat['bonus_equipment'] + 
                                stat['bonus_temporary'])
                        st.write(f"{stat['stat_name']}: {total} "
                                f"(Base: {stat['base_value']})")
                
                # Class Information
                st.subheader("Classes")
                classes = load_character_class_info(character.id)
                if classes:
                    for class_info in classes:
                        with st.expander(f"{class_info['class_name']} "
                                       f"(Level {class_info['current_level']})"):
                            st.write(f"Type: {class_info['class_type']}")
                            st.write(f"Experience: {class_info['current_exp']}")
                            if class_info['is_active']:
                                st.write("**Currently Active**")
                
                # Abilities
                st.subheader("Abilities")
                abilities = load_active_abilities(character.id)
                if abilities:
                    for ability in abilities:
                        with st.expander(ability['ability_name']):
                            st.write(ability['description'])
                            cols = st.columns(4)
                            if ability['mana_cost']:
                                cols[0].write(f"Mana Cost: {ability['mana_cost']}")
                            if ability['cooldown']:
                                cols[1].write(f"Cooldown: {ability['cooldown']}")
                            if ability['damage']:
                                cols[2].write(f"Damage: {ability['damage']}")
                            if ability['healing']:
                                cols[3].write(f"Healing: {ability['healing']}")
                            if ability['current_cooldown'] > 0:
                                st.write(f"On Cooldown: {ability['current_cooldown']} turns remaining")
                else:
                    st.write("No abilities unlocked")
                
                # Description if available
                if character.description:
                    st.subheader("Description")
                    st.write(character.description)
                
                # Character metadata in expandable section
                with st.expander("Character Metadata"):
                    st.write(f"Created: {character.created_at}")
                    st.write(f"Last Updated: {character.updated_at}")
                    if not character.is_active:
                        st.warning("This character is currently inactive")
    else:
        st.info("No characters found. Please create a character first.")