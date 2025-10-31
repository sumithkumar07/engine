"""
AI Movie Studio - AI Service Entry Point

This service provides natural language processing and intelligent scene control
for the AI Movie Studio application.
"""

import asyncio
import uvicorn
from fastapi import FastAPI
from loguru import logger
from dotenv import load_dotenv
import os

# Load environment variables
load_dotenv()

# Import service modules
from api import router as api_router
from ollama_client import OllamaClient  # NEW: Ollama LLM
from nlp_engine import NLPEngine
from scene_analyzer import SceneAnalyzer
from decision_engine import DecisionEngine

# Create FastAPI app
app = FastAPI(
    title="AI Movie Studio Service",
    description="AI-powered 3D movie creation with Ollama LLM",
    version="2.0.0"
)

# Global service instances
ollama_client = None  # NEW: Primary LLM
nlp_engine = None  # Fallback
scene_analyzer = None
decision_engine = None


@app.on_event("startup")
async def startup_event():
    """Initialize AI services on startup"""
    global ollama_client, nlp_engine, scene_analyzer, decision_engine
    
    logger.info("=== AI Movie Studio Service Starting ===")
    
    # Try to initialize Ollama LLM (Primary)
    try:
        logger.info("🤖 Initializing Ollama LLM...")
        ollama_model = os.getenv("OLLAMA_MODEL", "llama3:latest")  # default aligns with run.ps1
        ollama_host = os.getenv("OLLAMA_HOST", "http://localhost:11434")
        
        ollama_client = OllamaClient(model=ollama_model, host=ollama_host)
        ollama_client.initialize()
        
        logger.info(f"✅ Ollama LLM initialized successfully! Model: {ollama_model}")
    except Exception as e:
        logger.warning(f"⚠️ Ollama not available: {e}")
        logger.info("📝 Will use fallback spaCy NLP (install Ollama for better AI)")
        ollama_client = None
    
    # Initialize Fallback NLP Engine
    logger.info("Initializing Fallback NLP Engine...")
    nlp_engine = NLPEngine()
    await nlp_engine.initialize()
    logger.info("NLP Engine initialized")
    
    # Initialize Scene Analyzer
    logger.info("Initializing Scene Analyzer...")
    scene_analyzer = SceneAnalyzer()
    await scene_analyzer.initialize()
    logger.info("Scene Analyzer initialized")
    
    # Initialize Decision Engine
    logger.info("Initializing Decision Engine...")
    decision_engine = DecisionEngine()
    await decision_engine.initialize()
    logger.info("Decision Engine initialized")
    
    if ollama_client and ollama_client.initialized:
        logger.info("🎬 === AI Movie Studio Service Ready (Ollama LLM) ===")
    else:
        logger.info("🎬 === AI Movie Studio Service Ready (Fallback NLP) ===")
    logger.info("Tip: Install Ollama from https://ollama.ai for better AI!")


@app.on_event("shutdown")
async def shutdown_event():
    """Cleanup on shutdown"""
    logger.info("=== AI Movie Studio Service Shutting Down ===")
    
    if ollama_client:
        ollama_client.shutdown()
    if nlp_engine:
        await nlp_engine.shutdown()
    if scene_analyzer:
        await scene_analyzer.shutdown()
    if decision_engine:
        await decision_engine.shutdown()
    
    logger.info("=== AI Movie Studio Service Stopped ===")


# Include API routes
app.include_router(api_router, prefix="/api/v1")


@app.get("/")
async def root():
    """Health check endpoint"""
    return {
        "service": "AI Movie Studio",
        "status": "running",
        "version": "1.0.0"
    }


@app.get("/health")
async def health():
    """Health check endpoint"""
    return {
        "status": "healthy",
        "llm": {
            "provider": "Ollama" if (ollama_client and ollama_client.initialized) else "spaCy Fallback",
            "model": ollama_client.model if (ollama_client and ollama_client.initialized) else "en_core_web_sm",
            "available": ollama_client is not None and ollama_client.initialized
        },
        "nlp_engine": nlp_engine is not None,
        "scene_analyzer": scene_analyzer is not None,
        "decision_engine": decision_engine is not None
    }


# NEW: Expose AI components to API
def get_ai_components():
    """Get current AI components"""
    return {
        "ollama": ollama_client,
        "nlp": nlp_engine,
        "analyzer": scene_analyzer,
        "decision": decision_engine
    }


def main():
    """Main entry point"""
    # Get configuration from environment
    host = os.getenv("AI_SERVICE_HOST", "127.0.0.1")
    port = int(os.getenv("AI_SERVICE_PORT", "8080"))
    log_dir = os.getenv("LOG_DIR", "logs")
    
    # Create log directory if it doesn't exist
    log_path = os.path.join(os.path.dirname(__file__), log_dir)
    try:
        os.makedirs(log_path, exist_ok=True)
    except Exception as e:
        print(f"Warning: Could not create log directory {log_path}: {e}")
        print("Logging to console only")
        log_path = None
    
    # Configure logging
    if log_path:
        try:
            logger.add(
                os.path.join(log_path, "ai_service_{time}.log"),
                rotation="1 day",
                retention="7 days",
                level=os.getenv("LOG_LEVEL", "INFO")
            )
        except Exception as e:
            print(f"Warning: Could not configure file logging: {e}")
            print("Logging to console only")
    
    logger.info(f"Starting AI Service on {host}:{port}")
    
    # Run server
    uvicorn.run(
        app,
        host=host,
        port=port,
        log_level="info"
    )


if __name__ == "__main__":
    main()

