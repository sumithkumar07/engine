"""
Ollama LLM Client for AI Movie Studio

Handles communication with Ollama for natural language understanding
"""

import ollama
from typing import Dict, List, Any, Optional
from loguru import logger
import json


class OllamaClient:
    """Client for Ollama LLM"""
    
    def __init__(self, model: str = "llama2", host: str = "http://localhost:11434"):
        """
        Initialize Ollama client
        
        Args:
            model: Ollama model to use (llama2, mistral, codellama, etc.)
            host: Ollama server URL
        """
        self.model = model
        self.host = host
        self.client = ollama.Client(host=host)
        self.initialized = False
        
        # System prompt for 3D scene understanding
        self.system_prompt = """You are an AI assistant for a 3D movie creation tool. 
Your job is to understand natural language commands and convert them into structured JSON actions.

Available object types: cube, sphere, pyramid, chair, table, sofa, camera, light
Available actions: create, move, rotate, scale, delete
Position format: [x, y, z] where y is up

Examples:
- "Add a camera in front of character" -> {"intent": "create_camera", "position": [0, 1.5, 5], "look_at": [0, 0, 0]}
- "Create a living room" -> {"intent": "create_scene", "scene_type": "living_room"}
- "Move the cube to the right" -> {"intent": "move_object", "object": "cube", "direction": "right"}

Always respond with valid JSON only. No explanations."""
        
    def initialize(self):
        """Initialize and test Ollama connection"""
        try:
            logger.info(f"Connecting to Ollama at {self.host}...")
            
            # Test connection
            models = self.list_models()
            logger.info(f"Available Ollama models: {models}")
            
            # Check if our model is available
            if self.model not in models:
                logger.warning(f"Model {self.model} not found. Available: {models}")
                logger.info(f"Pulling model {self.model}...")
                self.pull_model(self.model)
            
            self.initialized = True
            logger.info(f"Ollama client initialized with model: {self.model}")
            
        except Exception as e:
            logger.error(f"Failed to initialize Ollama: {e}")
            logger.warning("Ollama not available - will use fallback NLP")
            self.initialized = False
    
    def list_models(self) -> List[str]:
        """List available Ollama models"""
        try:
            response = self.client.list()
            return [model['name'] for model in response['models']]
        except Exception as e:
            logger.error(f"Error listing models: {e}")
            return []
    
    def pull_model(self, model: str):
        """Pull a model from Ollama"""
        try:
            logger.info(f"Pulling Ollama model: {model}...")
            self.client.pull(model)
            logger.info(f"Model {model} pulled successfully")
        except Exception as e:
            logger.error(f"Error pulling model: {e}")
            raise
    
    def parse_command(self, command: str, context: Optional[Dict] = None) -> Dict[str, Any]:
        """
        Parse natural language command using Ollama
        
        Args:
            command: Natural language command
            context: Optional scene context
            
        Returns:
            Parsed command with intent and parameters
        """
        if not self.initialized:
            raise RuntimeError("Ollama client not initialized")
        
        try:
            logger.info(f"Parsing command with Ollama: '{command}'")
            
            # Build prompt with context
            prompt = f"""Command: "{command}"

Scene context: {json.dumps(context) if context else "empty scene"}

Parse this command into JSON format with intent and parameters."""
            
            # Call Ollama
            response = self.client.chat(
                model=self.model,
                messages=[
                    {
                        'role': 'system',
                        'content': self.system_prompt
                    },
                    {
                        'role': 'user',
                        'content': prompt
                    }
                ],
                options={
                    'temperature': 0.1,  # Low temperature for consistent parsing
                    'top_p': 0.9,
                }
            )
            
            # Extract response
            llm_response = response['message']['content']
            logger.info(f"Ollama raw response: {llm_response}")
            
            # Parse JSON from response
            try:
                # Try to extract JSON from response
                if '```json' in llm_response:
                    json_str = llm_response.split('```json')[1].split('```')[0].strip()
                elif '{' in llm_response:
                    # Extract JSON object
                    start = llm_response.index('{')
                    end = llm_response.rindex('}') + 1
                    json_str = llm_response[start:end]
                else:
                    json_str = llm_response
                
                result = json.loads(json_str)
                result['original_command'] = command
                result['llm_used'] = self.model
                
                logger.info(f"Parsed result: {result}")
                return result
                
            except json.JSONDecodeError as e:
                logger.error(f"Failed to parse JSON from Ollama response: {e}")
                logger.error(f"Raw response: {llm_response}")
                
                # Return fallback structure
                return {
                    "intent": "unknown",
                    "original_command": command,
                    "error": "Failed to parse LLM response",
                    "raw_response": llm_response
                }
                
        except Exception as e:
            logger.error(f"Error calling Ollama: {e}")
            raise
    
    def generate_suggestions(self, partial_command: str, context: Optional[Dict] = None) -> List[str]:
        """
        Generate command suggestions using Ollama
        
        Args:
            partial_command: Partial command to complete
            context: Optional scene context
            
        Returns:
            List of suggested completions
        """
        if not self.initialized:
            return []
        
        try:
            prompt = f"""Given the partial command: "{partial_command}"
And scene context: {json.dumps(context) if context else "empty scene"}

Suggest 5 complete commands the user might want to execute.
Return as JSON array of strings."""
            
            response = self.client.chat(
                model=self.model,
                messages=[
                    {
                        'role': 'system',
                        'content': self.system_prompt
                    },
                    {
                        'role': 'user',
                        'content': prompt
                    }
                ],
                options={
                    'temperature': 0.7,  # Higher temperature for variety
                }
            )
            
            llm_response = response['message']['content']
            
            # Parse suggestions
            try:
                if '```json' in llm_response:
                    json_str = llm_response.split('```json')[1].split('```')[0].strip()
                elif '[' in llm_response:
                    start = llm_response.index('[')
                    end = llm_response.rindex(']') + 1
                    json_str = llm_response[start:end]
                else:
                    json_str = llm_response
                
                suggestions = json.loads(json_str)
                return suggestions[:5]
                
            except (json.JSONDecodeError, ValueError, IndexError) as e:
                # Return default suggestions on parse error
                logger.warning(f"Failed to parse suggestions from LLM: {e}")
                return [
                    "Add a camera in front of character",
                    "Create a living room with furniture",
                    "Add dramatic lighting",
                    "Move the object to the left",
                    "Add a table in the center"
                ]
                
        except Exception as e:
            logger.error(f"Error generating suggestions: {e}")
            return []
    
    def shutdown(self):
        """Cleanup resources"""
        logger.info("Ollama client shutdown")
        self.initialized = False

