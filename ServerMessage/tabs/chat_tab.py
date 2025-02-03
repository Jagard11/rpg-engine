# ./ServerMessage/tabs/chat_interface.py

import streamlit as st
import requests
import json

def show_chat_interface(base_url: str):
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

    # Advanced settings in an expander
    with st.expander("Advanced Settings"):
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
        _handle_message_submission(base_url, instructions, server_message, temperature, max_tokens)

def _handle_message_submission(base_url: str, instructions: str, server_message: str, 
                             temperature: float, max_tokens: int):
    """Handle the submission of a message to the server"""
    with st.spinner("Sending message to server..."):
        # Construct the prompt by combining instructions and message
        full_prompt = ""
        if instructions:
            full_prompt = f"Instructions: {instructions}\n\n"
        full_prompt += server_message

        # Add previous context from chat history
        if st.session_state.chat_history:
            context = "\nPrevious conversation:\n"
            for msg in st.session_state.chat_history:
                prefix = "User:" if msg["is_user"] else "Assistant:"
                context += f"{prefix} {msg['content']}\n"
            full_prompt = context + "\nCurrent message:\n" + full_prompt

        # Construct the payload
        payload = {
            "prompt": full_prompt,
            "max_tokens": max_tokens,
            "temperature": temperature,
            "stream": False
        }
        
        # Debug information
        st.write("Debug: Sending the following payload:")
        st.json(payload)
        
        try:
            # Set headers explicitly
            headers = {
                "Content-Type": "application/json"
            }
            
            # Make the request
            response = requests.post(
                f"{base_url}/completions",
                headers=headers,
                json=payload,
                verify=False
            )
            
            # Log response information
            st.write(f"Debug: Response status code: {response.status_code}")
            st.write("Debug: Response headers:", dict(response.headers))
            
            if response.status_code == 200:
                _handle_successful_response(response, server_message)
            else:
                _handle_error_response(response)
        except Exception as e:
            _handle_exception(e)

def _handle_successful_response(response, server_message):
    """Handle a successful response from the server"""
    response_data = response.json()
    
    # Extract the response content
    assistant_message = response_data['choices'][0]['text']
    
    # Add messages to chat history
    st.session_state.chat_history.extend([
        {"content": server_message, "is_user": True},
        {"content": assistant_message, "is_user": False}
    ])
    
    st.success("Received response from server:")
    st.write(assistant_message)
    st.divider()
    st.json(response_data)

def _handle_error_response(response):
    """Handle an error response from the server"""
    st.error(f"Server error: {response.status_code}")
    st.write("Response content:")
    try:
        st.json(response.json())
    except:
        st.write(response.text)

def _handle_exception(e):
    """Handle exceptions during the request"""
    st.error(f"Exception occurred: {str(e)}")
    import traceback
    st.code(traceback.format_exc())