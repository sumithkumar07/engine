"""
API Routes for AI Movie Studio Service
"""

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel
from typing import List, Dict, Any, Optional
from loguru import logger

router = APIRouter()


# ============================================================================
# Request/Response Models
# ============================================================================

class CommandRequest(BaseModel):
    """Natural language command from user"""
    command: str
    scene_state: Optional[Dict[str, Any]] = None
    user_context: Optional[Dict[str, Any]] = None


class CommandResponse(BaseModel):
    """Response with parsed command and actions"""
    success: bool
    message: str
    actions: List[Dict[str, Any]]
    suggestions: Optional[List[str]] = None


class SceneStateUpdate(BaseModel):
    """Scene state update from engine"""
    objects: List[Dict[str, Any]]
    lights: List[Dict[str, Any]]
    cameras: List[Dict[str, Any]]
    timestamp: float


class SuggestionRequest(BaseModel):
    """Request for AI suggestions"""
    partial_command: str
    scene_state: Optional[Dict[str, Any]] = None


class SuggestionResponse(BaseModel):
    """AI suggestions response"""
    suggestions: List[str]
    completions: List[str]


# ============================================================================
# API Endpoints
# ============================================================================

@router.post("/command", response_model=CommandResponse)
async def process_command(request: CommandRequest):
    """
    Process a natural language command using Ollama LLM or fallback NLP
    
    Args:
        request: CommandRequest with user command
        
    Returns:
        CommandResponse with parsed actions
    """
    try:
        logger.info(f"📝 Processing command: {request.command}")
        
        from main import get_ai_components
        components = get_ai_components()
        
        ollama_client = components["ollama"]
        nlp_engine = components["nlp"]
        decision_engine = components["decision"]
        
        # Try Ollama LLM first (better AI)
        if ollama_client and ollama_client.initialized:
            logger.info("🤖 Using Ollama LLM for command parsing")
            parsed = ollama_client.parse_command(request.command, request.scene_state)
        else:
            # Fallback to spaCy NLP
            logger.info("📝 Using fallback spaCy NLP")
            parsed = await nlp_engine.parse_command(request.command, request.scene_state)
        
        # Make decision based on parsed intent
        decision = await decision_engine.make_decision(
            parsed.get("intent", "unknown"),
            {
                **parsed.get("parameters", {}),
                **(request.scene_state if request.scene_state else {})
            }
        )
        
        # Generate suggestions
        if ollama_client and ollama_client.initialized:
            suggestions = ollama_client.generate_suggestions(request.command, request.scene_state)
        else:
            suggestions = await nlp_engine.generate_suggestions(request.command)
        
        return CommandResponse(
            success=True,
            message=decision.get("reasoning", "Command processed"),
            actions=decision.get("actions", []),
            suggestions=suggestions
        )
        
    except Exception as e:
        logger.error(f"Error processing command: {e}")
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/scene/update")
async def update_scene_state(state: SceneStateUpdate):
    """
    Update AI's knowledge of current scene state
    
    Args:
        state: Current scene state from engine
        
    Returns:
        Success message
    """
    try:
        logger.info(f"Scene state updated: {len(state.objects)} objects, "
                   f"{len(state.lights)} lights, {len(state.cameras)} cameras")
        
        # Update scene analyzer with new state
        from main import get_ai_components
        components = get_ai_components()
        scene_analyzer = components["analyzer"]
        
        if scene_analyzer and scene_analyzer.initialized:
            await scene_analyzer.update_scene_state({
                "objects": state.objects,
                "lights": state.lights,
                "cameras": state.cameras,
                "timestamp": state.timestamp
            })
            logger.info("Scene analyzer updated successfully")
        else:
            logger.warning("Scene analyzer not available")
        
        return {"success": True, "message": "Scene state updated"}
        
    except Exception as e:
        logger.error(f"Error updating scene state: {e}")
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/suggestions", response_model=SuggestionResponse)
async def get_suggestions(request: SuggestionRequest):
    """
    Get real-time command suggestions
    
    Args:
        request: Partial command to complete
        
    Returns:
        List of suggestions
    """
    try:
        logger.info(f"Getting suggestions for: {request.partial_command}")
        
        # TODO: Implement actual suggestion engine
        # For now, return mock suggestions
        suggestions = [
            "Add a camera in front of character",
            "Add dramatic lighting to the scene",
            "Create a living room with furniture"
        ]
        
        completions = [
            "Add a table",
            "Add a chair",
            "Add a light"
        ]
        
        return SuggestionResponse(
            suggestions=suggestions,
            completions=completions
        )
        
    except Exception as e:
        logger.error(f"Error getting suggestions: {e}")
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/templates")
async def get_templates():
    """
    Get available scene templates
    
    Returns:
        List of available templates
    """
    try:
        # TODO: Load from database or configuration
        templates = {
            "scenes": [
                "Living Room",
                "Bedroom",
                "Office",
                "Kitchen"
            ],
            "cameras": [
                "Close-up",
                "Medium Shot",
                "Wide Shot",
                "Tracking Shot"
            ],
            "lighting": [
                "Dramatic",
                "Romantic",
                "Action",
                "Soft"
            ]
        }
        
        return templates
        
    except Exception as e:
        logger.error(f"Error getting templates: {e}")
        raise HTTPException(status_code=500, detail=str(e))


@router.post("/learn")
async def learn_from_feedback(feedback: Dict[str, Any]):
    """
    Learn from user feedback
    
    Args:
        feedback: User feedback data
        
    Returns:
        Success message
    """
    try:
        logger.info(f"Learning from feedback: {feedback}")
        
        # TODO: Implement learning system
        
        return {"success": True, "message": "Feedback recorded"}
        
    except Exception as e:
        logger.error(f"Error learning from feedback: {e}")
        raise HTTPException(status_code=500, detail=str(e))

