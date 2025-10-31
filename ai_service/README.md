# AI Movie Studio - AI Service

## Overview

The AI Service is a FastAPI-based backend that provides natural language processing and intelligent scene control for the AI Movie Studio application. It uses Ollama LLM as the primary AI engine with spaCy NLP as a fallback.

## Features

- **Natural Language Command Processing**: Parse user commands like "Add a camera in front of character"
- **Ollama LLM Integration**: Primary AI for intelligent command understanding
- **Fallback NLP**: spaCy-based rule system when Ollama is unavailable
- **Scene Analysis**: Understand and analyze 3D scene state
- **Decision Engine**: Make intelligent decisions for object placement, lighting, and camera positioning
- **Real-time Suggestions**: Provide command suggestions as users type
- **Template System**: Pre-built templates for scenes, cameras, and lighting
- **Learning System**: Record user feedback for future improvements

## Installation

### Prerequisites

- Python 3.9 or later
- Ollama (optional, for better AI - download from https://ollama.ai)

### Setup

1. **Create virtual environment**:
   ```bash
   python -m venv .venv
   ```

2. **Activate virtual environment**:
   - Windows: `.venv\Scripts\Activate.ps1`
   - Linux/Mac: `source .venv/bin/activate`

3. **Install dependencies**:
   ```bash
   pip install -r requirements.txt
   ```

4. **Download spaCy model** (for fallback NLP):
   ```bash
   python -m spacy download en_core_web_sm
   ```

5. **Configure environment** (optional):
   ```bash
   cp .env.example .env
   # Edit .env with your settings
   ```

### Running with Ollama (Recommended)

1. **Install Ollama** from https://ollama.ai

2. **Pull the model**:
   ```bash
   ollama pull llama3:latest
   ```

3. **Start Ollama service** (if not auto-started):
   ```bash
   ollama serve
   ```

4. **Start AI service**:
   ```bash
   python main.py
   ```

### Running without Ollama (Fallback Mode)

If Ollama is not installed, the service will automatically use the spaCy fallback:

```bash
python main.py
```

## Configuration

Environment variables (in `.env` file or system):

```bash
# Ollama Configuration
OLLAMA_HOST=http://localhost:11434
OLLAMA_MODEL=llama3:latest

# AI Service Configuration
AI_SERVICE_HOST=127.0.0.1
AI_SERVICE_PORT=8080

# Logging Configuration
LOG_LEVEL=INFO
LOG_DIR=logs
```

## API Endpoints

### Health Check
```
GET /health
```
Returns service status and AI availability.

### Process Command
```
POST /api/v1/command
```
Process a natural language command.

Request body:
```json
{
  "command": "Add a camera in front of character",
  "scene_state": { /* optional scene context */ },
  "user_context": { /* optional user context */ }
}
```

Response:
```json
{
  "success": true,
  "message": "Command processed",
  "actions": [
    {
      "type": "create_camera",
      "position": [0, 1.5, 5],
      "look_at": [0, 0, 0]
    }
  ],
  "suggestions": ["Add lighting", "Adjust camera angle"]
}
```

### Update Scene State
```
POST /api/v1/scene/update
```
Update the AI's knowledge of current scene state.

### Get Suggestions
```
POST /api/v1/suggestions
```
Get real-time command suggestions.

### Get Templates
```
GET /api/v1/templates
```
Get available scene, camera, and lighting templates.

### Submit Feedback
```
POST /api/v1/learn
```
Submit user feedback for learning.

## Architecture

### Components

1. **OllamaClient** (`ollama_client.py`): 
   - Communicates with Ollama LLM
   - Parses commands using AI
   - Generates intelligent suggestions

2. **NLPEngine** (`nlp_engine.py`):
   - Fallback rule-based NLP using spaCy
   - Pattern matching for command intent
   - Entity extraction

3. **SceneAnalyzer** (`scene_analyzer.py`):
   - Analyzes 3D scene composition
   - Finds optimal object positions
   - Evaluates scene balance and lighting

4. **DecisionEngine** (`decision_engine.py`):
   - Makes intelligent decisions based on context
   - Applies cinematic rules
   - Manages templates

5. **API** (`api.py`):
   - FastAPI routes
   - Request/response models
   - Input validation

## Development

### Code Quality

All code follows:
- Type hints for all functions
- Comprehensive error handling
- Input validation
- Logging for debugging
- Docstrings for documentation

### Running Tests

```bash
# Unit tests
pytest tests/

# Integration tests
pytest tests/integration/

# Coverage report
pytest --cov=. tests/
```

### Linting

```bash
# Python linting
ruff check .

# Type checking
mypy .
```

## Troubleshooting

### Ollama Connection Failed

If you see "Ollama not available" warnings:
1. Ensure Ollama is installed
2. Check Ollama is running: `ollama list`
3. Verify OLLAMA_HOST in .env is correct
4. The service will work with fallback NLP

### spaCy Model Not Found

If spaCy model is missing:
```bash
python -m spacy download en_core_web_sm
```

### Port Already in Use

If port 8080 is occupied:
```bash
# Change port in .env
AI_SERVICE_PORT=8081
```

## Performance

- **With Ollama**: ~1-2 seconds per command (depends on model and hardware)
- **Fallback NLP**: ~50-100ms per command
- **Memory**: ~500MB with Ollama, ~200MB without

## Logging

Logs are written to:
- Console (stdout)
- `logs/ai_service_YYYY-MM-DD_HH-MM-SS.log`

Log rotation: Daily, kept for 7 days

## Security

- Input validation on all endpoints
- Command length limits (1000 chars)
- Scene size limits (1000 objects max)
- No external API calls (all processing local)

## Future Improvements

- [ ] Fine-tune Ollama model on movie production data
- [ ] Add persistent learning database
- [ ] Support for multiple languages
- [ ] Advanced template customization
- [ ] Real-time collaboration features
- [ ] Performance optimization with caching

## Support

For issues or questions:
1. Check logs in `logs/` directory
2. Review error messages in console
3. Ensure all dependencies are installed
4. Verify Ollama is running (if using)

## License

See LICENSE file in project root.

---

**AI Movie Studio AI Service** - Making movie creation intelligent and accessible.

