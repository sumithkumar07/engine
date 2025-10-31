# AI Movie Studio - Development Setup Guide

## **🚀 Quick Start**

### **Prerequisites**
- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with C++ development tools
- **CMake** 3.20 or later
- **DirectX 12** (included with Windows 10/11)

### **Build Instructions**

#### **Windows (Recommended)**
```bash
# Clone the repository
git clone <repository-url>
cd ai-movie-studio

# Run the build script
build.bat
```

#### **Manual Build**
```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# Build the project
cmake --build . --config Release
```

### **Run the Application**
```bash
cd build/bin/Release
AI_Movie_Studio.exe
```

## **📁 Project Structure**

```
AI_Movie_Studio/
├── 📁 engine/                    # C++ 3D Engine
│   ├── 📁 src/                   # Source Code
│   │   ├── 📁 core/              # Core systems
│   │   ├── 📁 rendering/          # DirectX 12 rendering
│   │   ├── 📁 scene/              # Scene management
│   │   ├── 📁 ui/                 # User interface
│   │   ├── 📁 audio/              # Audio system
│   │   └── 📁 animation/          # Animation system
│   ├── 📁 include/                # Header Files
│   └── 📁 assets/                 # Engine Assets
├── 📁 ai/                        # Python AI System
├── 📁 tools/                     # Development Tools
├── 📁 docs/                      # Documentation
└── 📁 tests/                     # Tests
```

## **🔧 Development Setup**

### **Visual Studio 2022 Setup**
1. Install **Visual Studio 2022 Community/Professional**
2. Select **C++ development tools** workload
3. Install **Windows 10/11 SDK**
4. Install **CMake tools for Visual Studio**

### **CMake Setup**
1. Download CMake from https://cmake.org/download/
2. Add CMake to system PATH
3. Verify installation: `cmake --version`

### **DirectX 12 Setup**
- DirectX 12 is included with Windows 10/11
- No additional installation required

## **🏗️ Building the Engine**

### **Core Components**
- **Engine Core**: Memory management, logging, resource management
- **Rendering**: DirectX 12 rendering pipeline
- **Scene Management**: 3D scene organization
- **User Interface**: 3D viewport and panels
- **Audio System**: 3D spatial audio
- **Animation System**: Character animation

### **Build Process**
1. **Configuration**: CMake generates Visual Studio solution
2. **Compilation**: C++ source files compiled to libraries
3. **Linking**: Libraries linked into executable
4. **Asset Copying**: Assets copied to output directory

## **🧪 Testing**

### **Unit Tests**
```bash
cd build
cmake --build . --target tests
ctest --output-on-failure
```

### **Integration Tests**
```bash
cd build/bin/Release
AI_Movie_Studio.exe --test-mode
```

## **🐛 Debugging**

### **Visual Studio Debugger**
1. Open `build/AI_Movie_Studio.sln` in Visual Studio
2. Set breakpoints in source code
3. Press F5 to start debugging

### **Logging**
- Logs are written to `ai_movie_studio.log`
- Console output shows real-time logs
- Log levels: Debug, Info, Warning, Error, Critical

## **📚 Documentation**

### **API Documentation**
- Engine API: `engine/docs/api_reference.md`
- AI API: `ai/docs/api_reference.md`
- Architecture: `docs/architecture.md`

### **Code Documentation**
- Header files contain detailed documentation
- Use Doxygen for generated documentation
- Run `doxygen Doxyfile` to generate docs

## **🚀 Next Steps**

### **Phase 1: Basic Engine (Current)**
- [x] Project structure setup
- [x] Core engine framework
- [x] DirectX 12 rendering pipeline
- [x] Basic scene management
- [x] Logging system
- [ ] Basic UI framework
- [ ] Asset loading system

### **Phase 2: AI Integration**
- [ ] Python AI system
- [ ] TCP/IP communication
- [ ] NLP command processing
- [ ] Scene analysis
- [ ] User-controlled AI

### **Phase 3: Advanced Features**
- [ ] Audio system
- [ ] Advanced animation
- [ ] Timeline system
- [ ] Post-processing
- [ ] Performance optimization

## **🤝 Contributing**

### **Code Style**
- **C++**: Follow Google C++ Style Guide
- **Python**: Follow PEP 8
- **Naming**: PascalCase for classes, camelCase for functions
- **Comments**: Use Doxygen-style comments

### **Git Workflow**
1. Create feature branch
2. Make changes
3. Write tests
4. Submit pull request
5. Code review
6. Merge to main

## **📞 Support**

### **Issues**
- Report bugs on GitHub Issues
- Include system information
- Provide reproduction steps

### **Documentation**
- Check `docs/` directory
- Read inline code comments
- Ask questions on Discussions

---

**🎬 Ready to start building the AI Movie Studio!**

**Quick Commands:**
```bash
# Build the project
build.bat

# Run the application
cd build/bin/Release
AI_Movie_Studio.exe

# Clean build
rm -rf build
mkdir build
```
