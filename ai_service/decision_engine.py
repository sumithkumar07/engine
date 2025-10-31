"""
Decision Engine for AI Movie Studio

Makes intelligent decisions based on context and scene state
"""

from typing import Dict, List, Any, Optional
from loguru import logger


class DecisionEngine:
    """Intelligent Decision Making Engine"""
    
    def __init__(self):
        self.rules = {}
        self.templates = {}
        self.user_preferences = {}
        self.initialized = False
        
    async def initialize(self):
        """Initialize decision engine"""
        try:
            logger.info("Initializing Decision Engine...")
            
            # Load decision rules
            self._load_rules()
            
            # Load templates
            self._load_templates()
            
            self.initialized = True
            logger.info("Decision Engine initialized successfully")
            
        except Exception as e:
            logger.error(f"Failed to initialize Decision Engine: {e}")
            raise
    
    async def shutdown(self):
        """Cleanup resources"""
        logger.info("Decision Engine shutdown")
        self.rules = {}
        self.templates = {}
        self.initialized = False
    
    def _load_rules(self):
        """Load decision rules"""
        self.rules = {
            # Object placement rules
            "placement": {
                "chair": {
                    "default_height": 0.0,
                    "preferred_grouping": True,
                    "relation_to_table": "around"
                },
                "table": {
                    "default_height": 0.0,
                    "preferred_grouping": True,
                    "acts_as_center": True
                },
                "light": {
                    "default_height": 2.0,
                    "recommended_count": {"min": 1, "max": 5},
                    "spacing": 3.0
                },
                "camera": {
                    "default_height": 1.5,
                    "recommended_distance": 5.0,
                    "look_at_center": True
                }
            },
            
            # Cinematic rules
            "composition": {
                "rule_of_thirds": True,
                "golden_ratio": True,
                "leading_lines": True
            },
            
            # Lighting rules
            "lighting": {
                "three_point_minimum": True,
                "key_light_intensity": 1.5,
                "fill_light_intensity": 0.7,
                "back_light_intensity": 1.0
            }
        }
    
    def _load_templates(self):
        """Load scene templates"""
        self.templates = {
            "living_room": {
                "objects": [
                    {"type": "sofa", "position": [0, 0, -2]},
                    {"type": "coffee_table", "position": [0, 0, 0]},
                    {"type": "chair", "position": [-2, 0, 0]},
                    {"type": "chair", "position": [2, 0, 0]}
                ],
                "lights": [
                    {"type": "key_light", "position": [3, 3, 3]},
                    {"type": "fill_light", "position": [-2, 2, 2]}
                ],
                "cameras": [
                    {"type": "wide_shot", "position": [0, 2, 8]}
                ]
            },
            
            "dramatic_lighting": {
                "lights": [
                    {"type": "key_light", "position": [4, 4, 4], "intensity": 2.0, "color": [1.0, 0.8, 0.6]},
                    {"type": "fill_light", "position": [-2, 2, 2], "intensity": 0.5},
                    {"type": "back_light", "position": [0, 3, -4], "intensity": 1.5}
                ]
            }
        }
    
    async def make_decision(self, intent: str, context: Dict[str, Any]) -> Dict[str, Any]:
        """
        Make intelligent decision based on intent and context
        
        Args:
            intent: Command intent
            context: Current scene context
            
        Returns:
            Decision with actions to take
        """
        if not self.initialized:
            raise RuntimeError("Decision Engine not initialized")
        
        try:
            logger.info(f"Making decision for intent: {intent}")
            
            if intent == "create_object":
                return await self._decide_object_creation(context)
            elif intent == "modify_object":
                return await self._decide_object_modification(context)
            elif intent == "create_camera":
                return await self._decide_camera_placement(context)
            elif intent == "create_light":
                return await self._decide_light_placement(context)
            elif intent == "create_scene":
                return await self._decide_scene_creation(context)
            else:
                return {"actions": [], "reasoning": "Unknown intent"}
                
        except Exception as e:
            logger.error(f"Error making decision: {e}")
            return {"actions": [], "error": str(e)}
    
    async def _decide_object_creation(self, context: Dict[str, Any]) -> Dict[str, Any]:
        """Decide how to create an object"""
        object_type = context.get("object_type", "cube")
        
        # Get placement rules
        rules = self.rules["placement"].get(object_type, {})
        
        # Determine position
        position = [0.0, rules.get("default_height", 0.0), 0.0]
        
        # Check for relative positioning
        if "relative_to" in context:
            # Position relative to another object
            reference_obj = context["relative_to"]
            position_hint = context.get("position_hint", {})
            
            # Use scene analyzer to find good position
            # (This would call scene_analyzer.find_free_position)
            position[0] += position_hint.get("x", 0.0) * 2.0
            position[2] += position_hint.get("z", 0.0) * 2.0
        
        return {
            "actions": [
                {
                    "type": "create_object",
                    "object_type": object_type,
                    "position": position,
                    "rotation": [0, 0, 0],
                    "scale": [1, 1, 1]
                }
            ],
            "reasoning": f"Creating {object_type} at optimal position"
        }
    
    async def _decide_object_modification(self, context: Dict[str, Any]) -> Dict[str, Any]:
        """Decide how to modify an object"""
        return {
            "actions": [
                {
                    "type": "modify_object",
                    "object_name": context.get("object_name"),
                    "property": context.get("property", "position"),
                    "value": context.get("value", [0, 0, 0])
                }
            ],
            "reasoning": "Modifying object as requested"
        }
    
    async def _decide_camera_placement(self, context: Dict[str, Any]) -> Dict[str, Any]:
        """Decide optimal camera placement"""
        camera_rules = self.rules["placement"]["camera"]
        
        # Default camera position
        distance = camera_rules["recommended_distance"]
        height = camera_rules["default_height"]
        
        # Position based on scene center
        position = [0, height, distance]
        look_at = [0, 0, 0]
        
        return {
            "actions": [
                {
                    "type": "create_camera",
                    "position": position,
                    "look_at": look_at,
                    "fov": 50.0
                }
            ],
            "reasoning": "Placing camera for optimal framing"
        }
    
    async def _decide_light_placement(self, context: Dict[str, Any]) -> Dict[str, Any]:
        """Decide optimal light placement"""
        light_template = context.get("template", "three_point")
        
        if light_template in self.templates:
            template = self.templates[light_template]
            actions = []
            
            for light in template.get("lights", []):
                actions.append({
                    "type": "create_light",
                    "light_type": light["type"],
                    "position": light["position"],
                    "intensity": light.get("intensity", 1.0),
                    "color": light.get("color", [1.0, 1.0, 1.0])
                })
            
            return {
                "actions": actions,
                "reasoning": f"Applied {light_template} lighting template"
            }
        
        # Default single light
        return {
            "actions": [
                {
                    "type": "create_light",
                    "light_type": "directional",
                    "position": [2, 2, 2],
                    "intensity": 1.0,
                    "color": [1.0, 1.0, 1.0]
                }
            ],
            "reasoning": "Creating default directional light"
        }
    
    async def _decide_scene_creation(self, context: Dict[str, Any]) -> Dict[str, Any]:
        """Decide how to create a scene from template"""
        scene_type = context.get("scene_type", "living_room")
        
        if scene_type in self.templates:
            template = self.templates[scene_type]
            actions = []
            
            # Add objects
            for obj in template.get("objects", []):
                actions.append({
                    "type": "create_object",
                    "object_type": obj["type"],
                    "position": obj["position"]
                })
            
            # Add lights
            for light in template.get("lights", []):
                actions.append({
                    "type": "create_light",
                    "light_type": light["type"],
                    "position": light["position"]
                })
            
            # Add cameras
            for camera in template.get("cameras", []):
                actions.append({
                    "type": "create_camera",
                    "position": camera["position"]
                })
            
            return {
                "actions": actions,
                "reasoning": f"Applied {scene_type} scene template"
            }
        
        return {
            "actions": [],
            "reasoning": f"Template {scene_type} not found"
        }

