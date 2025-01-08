import requests
import json
from datetime import datetime

def test_custom_character():
    """Test using a character defined in the system message"""
    URL = "http://127.0.0.1:5000/v1/chat/completions"
    
    request_data = {
        "messages": [
            {
                "role": "system",
                "content": "You are a friendly tavern keeper named Gus. You run the Golden Goblet tavern and are known for your welcoming nature and hearty laugh."
            },
            {
                "role": "user",
                "content": "Hello! Who are you?"
            }
        ],
        "mode": "instruct",  # Using instruct mode since it worked in the basic test
        "instruction_template": "Alpaca",
        "temperature": 0.7,
        "max_tokens": 250
    }
    
    try:
        print("\n=== Testing Custom Character Using System Message ===")
        print(f"\nSending request to {URL}")
        print("Request data:", json.dumps(request_data, indent=2))
        
        response = requests.post(
            URL,
            headers={"Content-Type": "application/json"},
            json=request_data,
            verify=False
        )
        
        print("\nStatus Code:", response.status_code)
        print("Response Headers:", dict(response.headers))
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        if response.status_code == 200:
            response_json = response.json()
            print("\nResponse Content:", json.dumps(response_json, indent=2))
            
            if 'choices' in response_json:
                print("\nGenerated Response:")
                print(response_json['choices'][0]['message']['content'])
            
            output = {
                "timestamp": timestamp,
                "test_type": "custom_character",
                "request": request_data,
                "response": response_json,
                "status_code": response.status_code
            }
        else:
            print("\nRaw Response:", response.text)
            output = {
                "timestamp": timestamp,
                "test_type": "custom_character",
                "request": request_data,
                "raw_response": response.text,
                "status_code": response.status_code,
                "headers": dict(response.headers)
            }
        
        # Save response to file
        filename = f"data/test_response_custom_character_{timestamp}.json"
        with open(filename, "w", encoding='utf-8') as f:
            json.dump(output, f, indent=4, ensure_ascii=False)
        print(f"\nFull results saved to {filename}")
        
    except Exception as e:
        print(f"\nAn error occurred: {str(e)}")
        print(f"Error type: {type(e)}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    try:
        test_custom_character()
    except Exception as e:
        print(f"\nMain execution error: {str(e)}")
        import traceback
        traceback.print_exc()
        print("\nMake sure oobabooga is running with the --api flag enabled")
        print("Example start command: python server.py --api")