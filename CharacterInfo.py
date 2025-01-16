# ./CharacterInfo.py

import streamlit as st
import sqlite3
from dataclasses import dataclass
from typing import List, Optional, Dict
from datetime import datetime
import json

@dataclass
class Character:
    id: int
    name: str
    class_id: int
    level: int
    experience: int
    current_health: int
    max_health: int
    current_mana: int
    max_mana: int
    strength: int
    dexterity: int
    intelligence: int
    constitution: int
    description: Optional[str] = None
    created_at: Optional[str] = None
    updated_at: Optional[str] = None

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
        cursor.execute("SELECT id, name FROM characters ORDER BY name")
        st.session_state.character_list = cursor.fetchall()
    except Exception as e:
        st.error(f"Error loading character list: {str(e)}")
    finally:
        conn.close()

def load_character(character_id: int) -> Optional[Character]:
    """Load a character from database"""
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

def load_character_abilities(character_id: int) -> List[Dict]:
    """Load abilities for a character"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("""
            SELECT a.* FROM abilities a
            JOIN character_abilities ca ON a.id = ca.ability_id
            WHERE ca.character_id = ?
        """, (character_id,))
        columns = [description[0] for description in cursor.description]
        return [dict(zip(columns, row)) for row in cursor.fetchall()]
    except Exception as e:
        st.error(f"Error loading character abilities: {str(e)}")
        return []
    finally:
        conn.close()

def load_character_class(class_id: int) -> Optional[Dict]:
    """Load class information"""
    conn = get_db_connection()
    cursor = conn.cursor()
    try:
        cursor.execute("SELECT * FROM character_classes WHERE id = ?", (class_id,))
        result = cursor.fetchone()
        if result:
            columns = [description[0] for description in cursor.description]
            return dict(zip(columns, result))
        return None
    except Exception as e:
        st.error(f"Error loading character class: {str(e)}")
        return None
    finally:
        conn.close()

def render_character_tab():
    """Display character information"""
    st.header("Character Information")
    
    # Character selection
    if st.session_state.character_list:
        selected_char = st.selectbox(
            "Select Character",
            options=st.session_state.character_list,
            format_func=lambda x: x[1],  # Display character name
            key="char_select"
        )
        
        if selected_char:
            character = load_character(selected_char[0])
            if character:
                st.session_state.current_character = character
                
                # Load additional character information
                character_class = load_character_class(character.class_id)
                abilities = load_character_abilities(character.id)
                
                # Display character info in columns
                col1, col2 = st.columns(2)
                with col1:
                    st.subheader("Basic Info")
                    st.write(f"Name: {character.name}")
                    if character_class:
                        st.write(f"Class: {character_class['name']}")
                    st.write(f"Level: {character.level}")
                    st.write(f"Experience: {character.experience}")
                    st.write(f"Health: {character.current_health}/{character.max_health}")
                    st.write(f"Mana: {character.current_mana}/{character.max_mana}")
                
                with col2:
                    st.subheader("Stats")
                    st.write(f"Strength: {character.strength}")
                    st.write(f"Dexterity: {character.dexterity}")
                    st.write(f"Intelligence: {character.intelligence}")
                    st.write(f"Constitution: {character.constitution}")
                
                # Display abilities
                st.subheader("Abilities")
                if abilities:
                    for ability in abilities:
                        with st.expander(ability['name']):
                            st.write(ability['description'])
                            if ability['mana_cost']:
                                st.write(f"Mana Cost: {ability['mana_cost']}")
                            if ability['cooldown']:
                                st.write(f"Cooldown: {ability['cooldown']} turns")
                            if ability['damage']:
                                st.write(f"Damage: {ability['damage']}")
                            if ability['healing']:
                                st.write(f"Healing: {ability['healing']}")
                else:
                    st.write("No abilities unlocked")
                
                # Display description
                if character.description:
                    st.subheader("Description")
                    st.write(character.description)
                
                # Display metadata
                with st.expander("Character Metadata"):
                    st.write(f"Created: {character.created_at}")
                    st.write(f"Last Updated: {character.updated_at}")
    else:
        st.info("No characters found. Please create a character first.")