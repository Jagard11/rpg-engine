import requests
import json
from datetime import datetime

def test_oobabooga_connection():
    # Gradio API endpoint for oobabooga
    URL = "http://127.0.0.1:7860/api/v1/chat"
    
    # Test prompt following Gradio format
    prompt_data = {
        "user_input": "You are a friendly tavern keeper. Greet a new customer who just walked in.",
        "max_new_tokens": 250,
        "auto_max_new_tokens": False,
        "max_tokens_second": 0,
        "history": {"internal": [], "visible": []},
        "mode": "chat",  # or "instruct" depending on your model
        "character": "Assistant",
        "instruction_template": "Vicuna-v1.1",  # adjust based on your model
        "your_name": "You",
        
        # Generation parameters
        "temperature": 0.7,
        "top_p": 0.9,
        "top_k": 40,
        "typical_p": 1,
        "repetition_penalty": 1.15,
        "encoder_repetition_penalty": 1,
        "top_k": 40,
        "min_length": 0,
        "no_repeat_ngram_size": 0,
        "num_beams": 1,
        "penalty_alpha": 0,
        "length_penalty": 1,
        "early_stopping": True,
        "seed": -1,
        "add_bos_token": True,
        "truncation_length": 2048,
        "ban_eos_token": False,
        "skip_special_tokens": True,
        "stopping_strings": []
    }
    
    try:
        # Make the API call
        print(f"\nAttempting to connect to {URL}")
        print("\nSending request with data:", json.dumps(prompt_data, indent=2))
        
        response = requests.post(URL, json=prompt_data)
        
        # Print raw response for debugging
        print("\nStatus Code:", response.status_code)
        print("\nResponse Headers:", dict(response.headers))
        print("\nRaw Response Content:", response.text[:500])  # First 500 chars in case it's very long
        
        # Create a timestamp for the filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Try to parse JSON response if possible
        try:
            response_json = response.json()
            output = {
                "timestamp": timestamp,
                "request": prompt_data,
                "response": response_json,
                "status_code": response.status_code
            }
        except json.JSONDecodeError:
            # If JSON parsing fails, store raw response
            output = {
                "timestamp": timestamp,
                "request": prompt_data,
                "raw_response": response.text,
                "status_code": response.status_code,
                "headers": dict(response.headers)
            }
        
        # Write to a file in the data directory
        with open(f"data/test_response_{timestamp}.json", "w") as f:
            json.dump(output, f, indent=4)
            
        print(f"\nTest completed. Response saved to data/test_response_{timestamp}.json")
        
    except requests.exceptions.ConnectionError:
        print("Error: Could not connect to the oobabooga server. Make sure it's running and the API is enabled.")
    except Exception as e:
        print(f"An error occurred: {str(e)}")
        print(f"Error type: {type(e)}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    test_oobabooga_connection()