import requests
from datetime import date, timedelta
import re
import json
import random
import concurrent.futures

# Global session setup
session = requests.Session()
session.headers.update({'User-Agent': 'Mozilla/5.0'})

def fetch_wordle_solution():
    today = date.today().isoformat()
    url = f"https://www.nytimes.com/svc/wordle/v2/{today}.json"
    try:
        response = session.get(url, timeout=5)
        data = response.json()
        return data['solution']
    except Exception:
        return None

def fetch_wordle_guesses():
    base_url = "https://www.nytimes.com/games/wordle/"
    try:
        response = session.get(base_url, timeout=5)
        scripts = re.findall(r'src="([^"]+\.js)"', response.text)
        
        for script_path in scripts:
            full_url = script_path if script_path.startswith('http') else base_url + script_path
            js_content = session.get(full_url, timeout=5).text
            found_arrays = re.findall(r'\["[a-z]{5}"(?:,"[a-z]{5}"){5000,}\]', js_content)
            
            for arr_str in found_arrays:
                words = json.loads(arr_str)
                if "aahed" in words:
                    return words
    except:
        pass
    return None



def main():
    answer = fetch_wordle_solution()
    if answer:
        with open("answer.txt", "w") as file:
            file.write(answer)

    guesses = fetch_wordle_guesses()
    if guesses:
        with open('valid_guesses.txt', 'w') as file:
            for word in sorted(guesses):
                file.write(f"{word}\n")
    

if __name__ == "__main__":
    main()