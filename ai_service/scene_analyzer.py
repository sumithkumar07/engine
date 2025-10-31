"""
Scene Analyzer for AI Movie Studio

Analyzes and understands 3D scene state
"""

from typing import Dict, List, Any, Optional
from loguru import logger
import math


class SceneAnalyzer:
    """Scene Analysis Engine"""
    
    def __init__(self):
        self.current_scene = None
        self.scene_history = []
        self.initialized = False
        
    async def initialize(self):
        """Initialize scene analyzer"""
        try:
            logger.info("Initializing Scene Analyzer...")
            
            self.current_scene = {
                "objects": {},
                "lights": {},
                "cameras": {},
                "metadata": {}
            }
            
            self.initialized = True
            logger.info("Scene Analyzer initialized successfully")
            
        except Exception as e:
            logger.error(f"Failed to initialize Scene Analyzer: {e}")
            raise
    
    async def shutdown(self):
        """Cleanup resources"""
        logger.info("Scene Analyzer shutdown")
        self.current_scene = None
        self.scene_history = []
        self.initialized = False
    
    async def update_scene_state(self, scene_data: Dict[str, Any]):
        """Update current scene state"""
        if not self.initialized:
            raise RuntimeError("Scene Analyzer not initialized")
        
        try:
            logger.info("Updating scene state...")
            
            # Update objects
            if "objects" in scene_data:
                self.current_scene["objects"] = {
                    obj["name"]: obj for obj in scene_data["objects"]
                }
            
            # Update lights
            if "lights" in scene_data:
                self.current_scene["lights"] = {
                    light["name"]: light for light in scene_data["lights"]
                }
            
            # Update cameras
            if "cameras" in scene_data:
                self.current_scene["cameras"] = {
                    cam["name"]: cam for cam in scene_data["cameras"]
                }
            
            # Save to history
            self.scene_history.append({
                "timestamp": scene_data.get("timestamp", 0),
                "snapshot": self.current_scene.copy()
            })
            
            # Keep only last 100 history entries
            if len(self.scene_history) > 100:
                self.scene_history = self.scene_history[-100:]
            
            logger.info(f"Scene state updated: {len(self.current_scene['objects'])} objects")
            
        except Exception as e:
            logger.error(f"Error updating scene state: {e}")
            raise
    
    def get_object_by_name(self, name: str) -> Optional[Dict[str, Any]]:
        """Get object by name"""
        return self.current_scene["objects"].get(name)
    
    def get_objects_by_type(self, object_type: str) -> List[Dict[str, Any]]:
        """Get all objects of a specific type"""
        return [
            obj for obj in self.current_scene["objects"].values()
            if obj.get("type") == object_type
        ]
    
    def get_nearest_object(self, position: List[float]) -> Optional[Dict[str, Any]]:
        """Find nearest object to a position"""
        min_distance = float('inf')
        nearest_obj = None
        
        for obj in self.current_scene["objects"].values():
            obj_pos = obj.get("position", [0, 0, 0])
            distance = self._calculate_distance(position, obj_pos)
            
            if distance < min_distance:
                min_distance = distance
                nearest_obj = obj
        
        return nearest_obj
    
    def find_free_position(self, reference_obj: Optional[str] = None, 
                          direction: str = "front") -> List[float]:
        """
        Find a free position for placing new objects
        
        Args:
            reference_obj: Reference object name
            direction: Direction from reference ("front", "left", "right", "behind")
            
        Returns:
            [x, y, z] position
        """
        if reference_obj and reference_obj in self.current_scene["objects"]:
            ref_pos = self.current_scene["objects"][reference_obj].get("position", [0, 0, 0])
            
            # Calculate offset based on direction
            offsets = {
                "front": [0, 0, 2],
                "behind": [0, 0, -2],
                "left": [-2, 0, 0],
                "right": [2, 0, 0],
                "above": [0, 2, 0],
                "below": [0, -2, 0]
            }
            
            offset = offsets.get(direction, [0, 0, 2])
            return [ref_pos[0] + offset[0], ref_pos[1] + offset[1], ref_pos[2] + offset[2]]
        
        # Default: find empty spot in grid
        occupied_positions = set()
        for obj in self.current_scene["objects"].values():
            pos = obj.get("position", [0, 0, 0])
            occupied_positions.add((round(pos[0]), round(pos[2])))
        
        # Search in expanding grid
        for radius in range(0, 10):
            for x in range(-radius, radius + 1):
                for z in range(-radius, radius + 1):
                    if (x, z) not in occupied_positions:
                        return [float(x * 2), 0.0, float(z * 2)]
        
        return [0.0, 0.0, 0.0]
    
    def analyze_scene_composition(self) -> Dict[str, Any]:
        """Analyze scene composition for cinematic quality"""
        analysis = {
            "object_count": len(self.current_scene["objects"]),
            "light_count": len(self.current_scene["lights"]),
            "camera_count": len(self.current_scene["cameras"]),
            "balance": self._analyze_balance(),
            "lighting_quality": self._analyze_lighting(),
            "suggestions": []
        }
        
        # Generate suggestions
        if analysis["light_count"] == 0:
            analysis["suggestions"].append("Add lighting to the scene")
        
        if analysis["camera_count"] == 0:
            analysis["suggestions"].append("Add a camera to frame the scene")
        
        return analysis
    
    def _analyze_balance(self) -> str:
        """Analyze object distribution balance"""
        if not self.current_scene["objects"]:
            return "empty"
        
        # Calculate center of mass
        total_x = sum(obj.get("position", [0, 0, 0])[0] 
                     for obj in self.current_scene["objects"].values())
        total_z = sum(obj.get("position", [0, 0, 0])[2] 
                     for obj in self.current_scene["objects"].values())
        
        count = len(self.current_scene["objects"])
        center_x = total_x / count
        center_z = total_z / count
        
        # Check if balanced (center near origin)
        if abs(center_x) < 1.0 and abs(center_z) < 1.0:
            return "balanced"
        else:
            return "unbalanced"
    
    def _analyze_lighting(self) -> str:
        """Analyze lighting quality"""
        light_count = len(self.current_scene["lights"])
        
        if light_count == 0:
            return "dark"
        elif light_count == 1:
            return "single_light"
        elif light_count <= 3:
            return "three_point"
        else:
            return "complex"
    
    def _calculate_distance(self, pos1: List[float], pos2: List[float]) -> float:
        """Calculate Euclidean distance between two positions"""
        return math.sqrt(
            (pos1[0] - pos2[0]) ** 2 +
            (pos1[1] - pos2[1]) ** 2 +
            (pos1[2] - pos2[2]) ** 2
        )

