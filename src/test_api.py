import requests
import json
from datetime import datetime

def test_oobabooga_connection():
    # OpenAI-compatible endpoint as specified in the wiki
    URL = "http://127.0.0.1:5000/v1/chat/completions"
    
    # Test message following the OpenAI chat format
    messages = [
        {
            "role": "user",
            "content": "You are a friendly tavern keeper. Greet a new customer who just walked in."
        }
    ]
    
    # Request body following OpenAI format
    request_data = {
        "messages": messages,
        "mode": "chat",         # can be chat or instruct
        "stream": False,        # we'll start with non-streaming for simplicity
        "max_tokens": 250,
        "temperature": 0.7,
        "top_p": 0.9
    }
    
    try:
        print(f"\nAttempting to connect to {URL}")
        print("\nSending request with data:", json.dumps(request_data, indent=2))
        
        # Make the API call
        response = requests.post(
            URL, 
            headers={"Content-Type": "application/json"},
            json=request_data,
            verify=False  # as mentioned in wiki for local connections
        )
        
        # Print response details for debugging
        print("\nStatus Code:", response.status_code)
        print("\nResponse Headers:", dict(response.headers))
        
        # Create a timestamp for the filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        try:
            response_json = response.json()
            print("\nResponse Content:", json.dumps(response_json, indent=2))
            
            output = {
                "timestamp": timestamp,
                "request": request_data,
                "response": response_json,
                "status_code": response.status_code
            }
            
            # If successful, print the generated text
            if response.status_code == 200:
                generated_text = response_json['choices'][0]['message']['content']
                print("\nGenerated Response:")
                print(generated_text)
                
        except json.JSONDecodeError:
            print("\nRaw Response Content (not JSON):", response.text)
            output = {
                "timestamp": timestamp,
                "request": request_data,
                "raw_response": response.text,
                "status_code": response.status_code,
                "headers": dict(response.headers)
            }
        
        # Write complete response to file
        with open(f"data/test_response_{timestamp}.json", "w", encoding='utf-8') as f:
            json.dump(output, f, indent=4, ensure_ascii=False)
            
        print(f"\nTest completed. Full results saved to data/test_response_{timestamp}.json")
        
    except requests.exceptions.ConnectionError as e:
        print(f"Connection Error: {str(e)}")
        print("\nError: Could not connect to the oobabooga API server.")
        print("Make sure oobabooga is running with the --api flag enabled.")
        print("Example start command: python server.py --api")
    except Exception as e:
        print(f"\nAn error occurred: {str(e)}")
        print(f"Error type: {type(e)}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    test_oobabooga_connection()