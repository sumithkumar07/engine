# 🚀 How to Run AI Movie Studio Properly

## ✅ **Option 1: Run App Only (No AI Features)**

The app will run, but AI commands won't work:

```powershell
# 1. Build the app
cmake --build build --config Debug

# 2. Run the app
.\build\bin\Debug\AI_Movie_Studio.exe
```

**What you'll see:**
- Console window with debug output
- Main GUI window with all UI panels
- AI panel will show "Failed to connect to AI service" - this is OK!

---

## 🎯 **Option 2: Run Full App with AI Features**

### **Step 1: Start Ollama (if you have it)**

Open a **new PowerShell window**:

```powershell
# Check if Ollama is running
ollama list

# If not running, start it
ollama serve

# Pull the model (first time only)
ollama pull llama3:latest
```

### **Step 2: Start AI Service (Python)**

Open **another PowerShell window** and run:

```powershell
cd C:\Users\sumit\OneDrive\Desktop\projects\engine

# Navigate to AI service
cd ai_service

# Activate virtual environment (create it first if needed)
.\.venv\Scripts\Activate.ps1

# If .venv doesn't exist, create it:
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt

# Set environment variables
$env:OLLAMA_HOST = "http://localhost:11434"
$env:OLLAMA_MODEL = "llama3:latest"
$env:AI_SERVICE_HOST = "127.0.0.1"
$env:AI_SERVICE_PORT = "8080"

# Start the AI service
python main.py
```

You should see:
```
🎬 === AI Movie Studio Service Ready ===
INFO:     Uvicorn running on http://127.0.0.1:8080
```

### **Step 3: Run the C++ App**

Open **another PowerShell window** and run:

```powershell
cd C:\Users\sumit\OneDrive\Desktop\projects\engine

# Build (if needed)
cmake --build build --config Debug

# Run
.\build\bin\Debug\AI_Movie_Studio.exe
```

---

## 🔄 **Option 3: Use the Automation Script**

Run everything with one command:

```powershell
.\run.ps1
```

This script will:
1. ✅ Check if Ollama is running
2. ✅ Setup Python virtual environment
3. ✅ Start AI service
4. ✅ (You'll need to manually start the C++ app in another terminal)

---

## 🐛 **Troubleshooting**

### **App exits immediately:**
- Check if windows are hidden: Press `Alt+Tab`
- Look for "AI Movie Studio V2.0" window
- Look for "AI Movie Studio V2.0 - Debug Console" window

### **Can't see console output:**
- The console window is separate from PowerShell
- Press `Alt+Tab` to find "AI Movie Studio V2.0 - Debug Console"
- The console shows all debug logs

### **AI service fails to start:**
- Make sure Ollama is running: `ollama list`
- Check Python is installed: `python --version`
- Install requirements: `pip install -r ai_service/requirements.txt`

### **AI features don't work (error 10061):**
- This is OK! The app runs without AI service
- AI panel will show connection errors, but other features work
- To fix: Start the AI service (see Step 2 above)

---

## 📋 **Quick Reference**

**Minimal (app only):**
```powershell
cmake --build build --config Debug
.\build\bin\Debug\AI_Movie_Studio.exe
```

**Full (with AI):**
```powershell
# Terminal 1: Start Ollama
ollama serve

# Terminal 2: Start AI Service
cd ai_service
.\.venv\Scripts\Activate.ps1
python main.py

# Terminal 3: Run App
cd C:\Users\sumit\OneDrive\Desktop\projects\engine
.\build\bin\Debug\AI_Movie_Studio.exe
```

