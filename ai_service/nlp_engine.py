"""
NLP Engine for AI Movie Studio

Handles natural language processing for command understanding
"""

try:
    import spacy  # Optional: only used if available
    SPACY_AVAILABLE = True
except ImportError:
    spacy = None
    SPACY_AVAILABLE = False
from typing import Dict, List, Any, Optional
from loguru import logger


class NLPEngine:
    """Natural Language Processing Engine"""
    
    def __init__(self):
        self.nlp = None
        self.command_patterns = {}
        self.initialized = False
        
    async def initialize(self):
        """Initialize NLP models"""
        try:
            if SPACY_AVAILABLE and spacy is not None:
                logger.info("Loading spaCy model...")
                # Load English language model
                try:
                    self.nlp = spacy.load("en_core_web_sm")
                    logger.info("spaCy model loaded successfully")
                except OSError:
                    logger.warning("spaCy model not found, will use fallback NLP")
                    self.nlp = None
            else:
                logger.warning("spaCy not available; using lightweight rule-based NLP fallback")
                self.nlp = None
            
            # Initialize command patterns
            self._initialize_patterns()
            
            self.initialized = True
            logger.info("NLP Engine initialized successfully")
            
        except Exception as e:
            logger.error(f"Failed to initialize NLP Engine: {e}")
            raise
    
    async def shutdown(self):
        """Cleanup resources"""
        logger.info("NLP Engine shutdown")
        self.nlp = None
        self.initialized = False
    
    def _initialize_patterns(self):
        """Initialize command pattern matching"""
        self.command_patterns = {
            # Object commands
            "create_object": [
                "add", "create", "make", "place", "put"
            ],
            "modify_object": [
                "move", "rotate", "scale", "change", "set", "adjust"
            ],
            "delete_object": [
                "remove", "delete", "destroy"
            ],
            
            # Camera commands
            "create_camera": [
                "add camera", "create camera", "place camera"
            ],
            "modify_camera": [
                "move camera", "rotate camera", "change camera"
            ],
            
            # Lighting commands
            "create_light": [
                "add light", "create light", "add lighting"
            ],
            "modify_light": [
                "change light", "adjust light", "set light"
            ],
            
            # Scene commands
            "create_scene": [
                "create scene", "make scene", "build scene"
            ],
            "save_scene": [
                "save", "save scene", "store scene"
            ],
            "load_scene": [
                "load", "load scene", "open scene"
            ]
        }
    
    def _create_fallback_token(self, text: str, index: int) -> Any:
        """
        Create a simple token object for fallback NLP when spaCy is not available
        
        Args:
            text: Token text
            index: Token index in sentence
            
        Returns:
            Simple token object with basic properties
        """
        verb_words = ["add", "create", "make", "move", "rotate", "scale", "remove", "delete"]
        
        class FallbackToken:
            def __init__(self, text: str, idx: int):
                self.text = text
                self.pos_ = "VERB" if text.lower() in verb_words else ""
                self.lemma_ = text.lower()
                self.dep_ = ""
                self.idx = idx
                self.head = type("TokenHead", (), {"text": ""})()
        
        return FallbackToken(text, index)
    
    def _create_fallback_doc(self, text: str, tokens: List[str]) -> Any:
        """
        Create a simple document object for fallback NLP when spaCy is not available
        
        Args:
            text: Original text
            tokens: List of token strings
            
        Returns:
            Simple document object compatible with spaCy API
        """
        class FallbackDoc:
            def __init__(self, text: str, tokens: List[str], token_creator):
                self.text = text
                self.ents = []
                self._tokens = [token_creator(t, i) for i, t in enumerate(tokens)]
            
            def __iter__(self):
                return iter(self._tokens)
        
        return FallbackDoc(text, tokens, self._create_fallback_token)
    
    async def parse_command(self, command: str, context: Optional[Dict] = None) -> Dict[str, Any]:
        """
        Parse natural language command
        
        Args:
            command: Natural language command
            context: Optional scene context
            
        Returns:
            Parsed command with intent and parameters
        """
        if not self.initialized:
            raise RuntimeError("NLP Engine not initialized")
        
        try:
            logger.info(f"Parsing command: '{command}'")
            
            # Process with spaCy if available, else use simple tokenization
            lower = command.lower()
            if self.nlp is not None:
                doc = self.nlp(lower)
                tokens = [token.text for token in doc]
            else:
                tokens = lower.split()
                doc = self._create_fallback_doc(lower, tokens)
            
            # Extract intent
            intent = self._extract_intent(doc)
            
            # Extract entities
            entities = self._extract_entities(doc)
            
            # Extract parameters
            parameters = self._extract_parameters(doc, entities)
            
            result = {
                "intent": intent,
                "entities": entities,
                "parameters": parameters,
                "original_command": command,
                "tokens": tokens
            }
            
            logger.info(f"Parsed result: {result}")
            return result
            
        except Exception as e:
            logger.error(f"Error parsing command: {e}")
            return {
                "intent": "unknown",
                "entities": [],
                "parameters": {},
                "original_command": command,
                "error": str(e)
            }
    
    def _extract_intent(self, doc) -> str:
        """Extract command intent"""
        # Check for command patterns
        text = doc.text
        
        for intent, patterns in self.command_patterns.items():
            for pattern in patterns:
                if pattern in text:
                    return intent
        
        # Check verbs for action intent
        verbs = [token.lemma_ for token in doc if token.pos_ == "VERB"]
        if verbs:
            if verbs[0] in ["add", "create", "make"]:
                return "create_object"
            elif verbs[0] in ["move", "rotate", "scale"]:
                return "modify_object"
            elif verbs[0] in ["remove", "delete"]:
                return "delete_object"
        
        return "unknown"
    
    def _extract_entities(self, doc) -> List[Dict[str, str]]:
        """Extract named entities"""
        entities = []
        
        # Extract spaCy entities
        for ent in doc.ents:
            entities.append({
                "text": ent.text,
                "type": ent.label_,
                "start": ent.start_char,
                "end": ent.end_char
            })
        
        # Extract object types
        object_types = ["cube", "sphere", "pyramid", "chair", "table", "camera", "light"]
        for token in doc:
            if token.text in object_types:
                entities.append({
                    "text": token.text,
                    "type": "OBJECT_TYPE",
                    "start": token.idx,
                    "end": token.idx + len(token.text)
                })
        
        return entities
    
    def _extract_parameters(self, doc, entities: List[Dict]) -> Dict[str, Any]:
        """Extract command parameters"""
        parameters = {}
        
        # Extract object type
        for entity in entities:
            if entity["type"] == "OBJECT_TYPE":
                parameters["object_type"] = entity["text"]
        
        # Extract position hints
        position_words = {
            "front": {"z": 1.0},
            "behind": {"z": -1.0},
            "left": {"x": -1.0},
            "right": {"x": 1.0},
            "above": {"y": 1.0},
            "below": {"y": -1.0}
        }
        
        for token in doc:
            if token.text in position_words:
                if "position_hint" not in parameters:
                    parameters["position_hint"] = {}
                parameters["position_hint"].update(position_words[token.text])
        
        # Extract relative positioning
        for token in doc:
            if token.dep_ == "pobj" and token.head.text == "of":
                parameters["relative_to"] = token.text
        
        return parameters
    
    async def generate_suggestions(self, partial_command: str) -> List[str]:
        """
        Generate command suggestions
        
        Args:
            partial_command: Partial command to complete
            
        Returns:
            List of suggested completions
        """
        suggestions = [
            "Add a camera in front of character",
            "Add dramatic lighting to the scene",
            "Create a living room with furniture",
            "Move the chair to the left of the table",
            "Add a table in the center",
            "Place a light above the character"
        ]
        
        # Filter suggestions based on partial command
        if partial_command:
            suggestions = [s for s in suggestions if partial_command.lower() in s.lower()]
        
        return suggestions[:5]  # Return top 5

