"""
API Routes for AI Movie Studio Service
"""

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, validator
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
    
    @validator('command')
    def validate_command(cls, v):
        """Validate command is not empty and within length limit"""
        if not v or not v.strip():
            raise ValueError('Command cannot be empty')
        if len(v) > 1000:
            raise ValueError('Command too long (max 1000 characters)')
        return v.strip()


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
    
    @validator('objects', 'lights', 'cameras')
    def validate_array_size(cls, v):
        """Validate arrays are not too large"""
        if len(v) > 1000:
            raise ValueError('Too many items (max 1000)')
        return v


class SuggestionRequest(BaseModel):
    """Request for AI suggestions"""
    partial_command: str
    scene_state: Optional[Dict[str, Any]] = None
    
    @validator('partial_command')
    def validate_partial_command(cls, v):
        """Validate partial command"""
        if len(v) > 500:
            raise ValueError('Partial command too long (max 500 characters)')
        return v.strip()


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
        
        from main import get_ai_components
        components = get_ai_components()
        
        ollama_client = components["ollama"]
        nlp_engine = components["nlp"]
        
        # Try Ollama first for better suggestions
        if ollama_client and ollama_client.initialized:
            suggestions = ollama_client.generate_suggestions(
                request.partial_command, 
                request.scene_state
            )
        elif nlp_engine and nlp_engine.initialized:
            suggestions = await nlp_engine.generate_suggestions(request.partial_command)
        else:
            # Fallback to default suggestions
            suggestions = [
                "Add a camera in front of character",
                "Add dramatic lighting to the scene",
                "Create a living room with furniture"
            ]
        
        # Generate completions based on partial command
        completions = []
        if request.partial_command:
            base_completions = [
                "Add a table",
                "Add a chair",
                "Add a light",
                "Move the cube",
                "Create a scene"
            ]
            completions = [
                c for c in base_completions 
                if c.lower().startswith(request.partial_command.lower())
            ][:3]
        
        return SuggestionResponse(
            suggestions=suggestions[:5],
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
        from main import get_ai_components
        components = get_ai_components()
        decision_engine = components["decision"]
        
        if decision_engine and decision_engine.initialized:
            # Get templates from decision engine
            templates = {
                "scenes": list(decision_engine.templates.keys()),
                "cameras": [
                    "Close-up",
                    "Medium Shot",
                    "Wide Shot",
                    "Tracking Shot",
                    "Over-the-shoulder"
                ],
                "lighting": [
                    "three_point",
                    "dramatic_lighting",
                    "Romantic",
                    "Action",
                    "Soft"
                ]
            }
        else:
            # Fallback templates
            templates = {
                "scenes": [
                    "living_room",
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


class FeedbackRequest(BaseModel):
    """User feedback data"""
    command: str
    result_quality: int  # 1-5 rating
    was_helpful: bool
    comments: Optional[str] = None
    timestamp: float


@router.post("/learn")
async def learn_from_feedback(feedback: FeedbackRequest):
    """
    Learn from user feedback
    
    Args:
        feedback: User feedback data
        
    Returns:
        Success message
    """
    try:
        logger.info(f"Learning from feedback: command='{feedback.command}', quality={feedback.result_quality}")
        
        # Store feedback for future learning
        # In production, this would save to a database
        feedback_data = {
            "command": feedback.command,
            "quality": feedback.result_quality,
            "helpful": feedback.was_helpful,
            "comments": feedback.comments,
            "timestamp": feedback.timestamp
        }
        
        # Log for analysis
        logger.info(f"Feedback recorded: {feedback_data}")
        
        # In future: Use this data to fine-tune the model or adjust decision rules
        # For now, just acknowledge receipt
        
        return {
            "success": True, 
            "message": "Feedback recorded for future improvements",
            "feedback_id": int(feedback.timestamp)
        }
        
    except Exception as e:
        logger.error(f"Error learning from feedback: {e}")
        raise HTTPException(status_code=500, detail=str(e))

