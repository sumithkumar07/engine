#!/usr/bin/env python3
"""
Simple test script to check if Ollama is working
"""

import ollama
import json

def test_ollama():
    try:
        print("Testing Ollama connection...")
        
        # Test connection
        client = ollama.Client(host='http://localhost:11434')
        
        # List models
        print("Available models:")
        models = client.list()
        for model in models['models']:
            print(f"  - {model['name']}")
        
        # Test with a simple prompt
        print("\nTesting with simple prompt...")
        response = client.chat(
            model='llama3:latest',
            messages=[
                {
                    'role': 'user',
                    'content': 'Hello, respond with just "OK"'
                }
            ]
        )
        
        print(f"Response: {response['message']['content']}")
        print("✅ Ollama is working!")
        return True
        
    except Exception as e:
        print(f"❌ Ollama test failed: {e}")
        print("\nTroubleshooting:")
        print("1. Make sure Ollama is running: ollama serve")
        print("2. Pull a model: ollama pull llama3:latest")
        print("3. Test manually: ollama run llama3:latest 'Hello'")
        return False

if __name__ == "__main__":
    test_ollama()
