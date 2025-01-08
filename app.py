import streamlit as st
import requests
import json
from datetime import datetime

# Initialize session state variables if they don't exist
if 'messages' not in st.session_state:
    st.session_state.messages = []

def send_message(messages, system_message, temperature, max_tokens):
    """Send message to the API and return the response"""
    URL = "http://127.0.0.1:5000/v1/chat/completions"
    
    if system_message:
        # Add system message at the beginning if it exists
        full_messages = [
            {"role": "system", "content": system_message}
        ] + messages
    else:
        full_messages = messages
    
    request_data = {
        "messages": full_messages,
        "mode": "instruct",
        "instruction_template": "Alpaca",
        "temperature": temperature,
        "max_tokens": max_tokens
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

# Streamlit UI
st.title("Chat with Oobabooga API")

# Sidebar controls
with st.sidebar:
    st.header("Settings")
    system_message = st.text_area(
        "System Message",
        value="You are a friendly tavern keeper named Gus. You run the Golden Goblet tavern and are known for your welcoming nature and hearty laugh.",
        help="Define the character's personality and role"
    )
    temperature = st.slider("Temperature", 0.0, 2.0, 0.7, 0.1)
    max_tokens = st.slider("Max Tokens", 50, 1000, 250, 50)
    
    if st.button("Clear Chat History"):
        st.session_state.messages = []
        st.rerun()

# Display chat messages
for message in st.session_state.messages:
    with st.chat_message(message["role"]):
        st.markdown(message["content"])

# Chat input
if prompt := st.chat_input("Type your message here..."):
    # Add user message to chat history
    st.session_state.messages.append({"role": "user", "content": prompt})
    
    # Get bot response
    response = send_message(
        st.session_state.messages,
        system_message,
        temperature,
        max_tokens
    )
    
    # Add assistant response to chat history
    st.session_state.messages.append({"role": "assistant", "content": response})
    
    # Rerun to update the chat display
    st.rerun()

# Footer
st.markdown("---")
st.markdown("*Using Oobabooga API with Streamlit*")