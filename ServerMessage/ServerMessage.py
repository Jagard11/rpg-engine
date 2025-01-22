# ./ServerMessage/ServerMessage.py

import streamlit as st
import requests
from datetime import datetime
import os
from enum import Enum
from pathlib import Path

# Enum for instruction types
class InstructionType(Enum):
    CONVERSATION = "Conversation"
    OPTION_SELECT = "OptionSelectPhase"
    ABILITY_SELECT = "AbilitySelectPhase"
    CONSEQUENCES = "ConsequencesPhase"

def init_server_state():
    """Initialize server-related session state variables"""
    if 'server_response' not in st.session_state:
        st.session_state.server_response = ""
    if 'server_address' not in st.session_state:
        st.session_state.server_address = "http://127.0.0.1:5000/v1/chat/completions"
    if 'selected_instruction' not in st.session_state:
        st.session_state.selected_instruction = ""
    if 'instruction_content' not in st.session_state:
        st.session_state.instruction_content = ""
    if 'message_data' not in st.session_state:
        st.session_state.message_data = ""

def render_server_tab():
    """Render the server communication tab"""
    # Initialize server state first
    init_server_state()

    st.title("Server Communication")

    # Server address input
    st.subheader("Server Configuration")
    server_address = st.text_input(
        "Server address",
        value=st.session_state.server_address,
        help="Enter the full URL of the server endpoint"
    )
    
    # Rest of your render_server_tab code remains the same...
    if server_address != st.session_state.server_address:
        st.session_state.server_address = server_address

    # Instruction selection
    st.subheader("Instruction Selection")
    instruction_type = st.selectbox(
        "Select instruction type",
        [inst.value for inst in InstructionType],
        key="instruction_selector"
    )

    # Update selected instruction when changed
    if instruction_type != st.session_state.selected_instruction:
        st.session_state.selected_instruction = instruction_type
        refresh_message_data()

    # Refresh button
    if st.button("Refresh Message Data"):
        refresh_message_data()

    # Display instruction content
    instruction_content = st.text_area(
        "Instruction Content",
        value=getattr(st.session_state, 'instruction_content', ''),
        height=100,
        key="instruction_display"
    )

    # Display message data
    st.subheader("Message Data")
    message_content = st.text_area(
        "Message Content",
        value=st.session_state.message_data,
        height=200,
        key="message_data_display"
    )

    # Send button
    if st.button("Send to Server"):
        # Use the current values from the text areas instead of session state
        response = send_to_server(instruction_content, message_content)
        st.session_state.server_response = response

    # Server Response
    st.subheader("Server Response")
    st.text_area("Latest Response",
                value=st.session_state.server_response,
                height=200)

# Keep your existing helper functions (load_instruction_file, load_message_data, send_to_server, refresh_message_data)
def load_instruction_file(instruction_type: InstructionType) -> str:
    """Load the content of the selected instruction file"""
    file_path = Path(f"ServerMessages/Instructions/{instruction_type.value}.txt")
    try:
        with open(file_path, 'r') as file:
            return file.read()
    except Exception as e:
        return f"Error loading instruction file: {str(e)}"

def load_message_data() -> str:
    """Load the content of the message data file"""
    try:
        with open("ServerMessages/MessageData/MessageData.txt", 'r') as file:
            return file.read()
    except Exception as e:
        return f"Error loading message data: {str(e)}"

def send_to_server(instruction_content: str, message_content: str):
    """Send message to the API and return the response"""
    URL = st.session_state.server_address
    
    full_message = f"{instruction_content}\n\n{message_content}"
    messages = [{"role": "user", "content": full_message}]
    request_data = {
        "messages": messages,
        "mode": "instruct",
        "instruction_template": "Alpaca",
        "temperature": 0.7,
        "max_tokens": 500
    }
    
    try:
        response = requests.post(
            URL,
            headers={"Content-Type": "application/json"},
            json=request_data,
            verify=False
        )
        
        if response.status_code == 200:
            return response.json()['choices'][0]['message']['content']
        else:
            return f"Error: {response.status_code} - {response.text}"
    except Exception as e:
        return f"Error: {str(e)}"

def refresh_message_data():
    """Refresh both instruction and message data fields"""
    if st.session_state.selected_instruction:
        st.session_state.instruction_content = load_instruction_file(
            InstructionType(st.session_state.selected_instruction)
        )
    st.session_state.message_data = load_message_data()