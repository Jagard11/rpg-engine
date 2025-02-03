# ./ServerMessage/tabs/chat_interface.py

import streamlit as st
import requests
import json
from typing import Dict, Any

def render_chat_tab(base_url: str):
    """Handle the chat interface tab functionality"""
    st.header("Character Communication")

    # Instructions field
    instructions = st.text_area(
        "Character Instructions",
        help="Specify instructions for how the character should behave and respond",
        height=100,
        key="char_instructions"
    )

    # Main message field
    server_message = st.text_area(
        "Message Content", 
        value="Hello!",
        help="The main message to send to the character",
        height=150,
        key="message_content"
    )

    # Termination instructions
    termination = st.text_area(
        "Termination Instructions",
        help="Specify conditions or instructions for ending the response",
        height=100,
        key="termination_instructions"
    )

    # Advanced settings in an expander
    with st.expander("Advanced Settings"):
        mode = st.radio(
            "Chat Mode",
            options=["chat", "instruct"],
            index=1,
            key="chat_mode"
        )
        
        instruction_template = st.text_input(
            "Instruction Template",
            value="Alpaca",
            help="Template to use for instruction formatting (e.g., Alpaca, Vicuna, etc.)",
            key="instruction_template"
        )
        
        temperature = st.slider(
            "Temperature",
            min_value=0.1,
            max_value=2.0,
            value=0.7,
            step=0.1,
            key="temp_slider"
        )
        max_tokens = st.number_input(
            "Max Tokens",
            min_value=1,
            max_value=2048,
            value=200,
            key="max_tokens_input"
        )

    if st.button("Send Message", key="send_msg_btn"):
        _handle_message_submission(
            base_url=base_url,
            instructions=instructions,
            server_message=server_message,
            termination=termination,
            mode=mode,
            instruction_template=instruction_template,
            temperature=temperature,
            max_tokens=max_tokens
        )

def _handle_message_submission(
    base_url: str,
    instructions: str,
    server_message: str,
    termination: str,
    mode: str,
    instruction_template: str,
    temperature: float,
    max_tokens: int
) -> None:
    """Handle the submission of a message to the server"""
    messages = []
    
    # Add system instruction if provided
    if instructions:
        messages.append({
            "role": "system",
            "content": instructions
        })
    
    # Add the current message
    messages.append({
        "role": "user",
        "content": server_message
    })
    
    # Add chat history
    for msg in st.session_state.chat_history:
        messages.append({
            "role": "user" if msg["is_user"] else "assistant",
            "content": msg["content"]
        })

    # Construct the payload
    payload = {
        "messages": messages,
        "mode": mode,
        "instruction_template": instruction_template,
        "max_tokens": max_tokens,
        "temperature": temperature,
    }
    
    if termination:
        payload["stop"] = [termination]
    
    try:
        _send_request_to_server(base_url, payload, server_message)
    except Exception as e:
        st.error(f"Exception occurred: {e}")
        import traceback
        st.code(traceback.format_exc())

def _send_request_to_server(base_url: str, payload: Dict[str, Any], server_message: str) -> None:
    """Send the request to the server and handle the response"""
    # Debug information
    st.write("Debug: Sending the following payload:")
    st.json(payload)
    
    response = requests.post(
        f"{base_url}/chat/completions",
        json=payload
    )

    if response.status_code == 200:
        _handle_successful_response(response, server_message)
    else:
        st.error(f"Server error: {response.status_code}\n{response.text}")

def _handle_successful_response(response: requests.Response, server_message: str) -> None:
    """Handle a successful response from the server"""
    response_data = response.json()
    
    # Extract the response content
    assistant_message = response_data['choices'][0]['message']['content']
    
    # Add messages to chat history
    st.session_state.chat_history.extend([
        {"content": server_message, "is_user": True},
        {"content": assistant_message, "is_user": False}
    ])
    
    st.success("Received response from server:")
    st.write(assistant_message)
    st.divider()
    st.json(response_data)