# 🎬 AI Movie Studio - Complete Installation Guide

## **📋 Prerequisites Overview**

To build and run the AI Movie Studio engine, you need to install:

1. **Visual Studio 2022** (or 2019) - C++ development tools
2. **CMake** - Build system generator
3. **Windows SDK** - Windows development tools
4. **DirectX 12** - Graphics API (included with Windows 10/11)

## **🔧 Detailed Installation Instructions**

### **1. Visual Studio 2022 (Recommended)**

#### **Download Link:**
- **Official Website**: https://visualstudio.microsoft.com/downloads/
- **Direct Download**: https://visualstudio.microsoft.com/vs/downloads/

#### **Installation Steps:**
1. **Download Visual Studio 2022 Community** (Free)
   - Go to the official website
   - Click "Download Visual Studio 2022 Community"
   - File size: ~3-5 GB

2. **Run the Installer**
   - Right-click the downloaded file
   - Select "Run as administrator"
   - Wait for the installer to load

3. **Select Workloads**
   - ✅ **Desktop development with C++** (Required)
   - ✅ **Game development with C++** (Optional, for DirectX tools)
   - ✅ **Universal Windows Platform development** (Optional)

4. **Individual Components (Important)**
   - ✅ **MSVC v143 - VS 2022 C++ x64/x86 build tools**
   - ✅ **Windows 11 SDK (10.0.22621.0)** or latest
   - ✅ **CMake tools for Visual Studio**
   - ✅ **Git for Windows**
   - ✅ **IntelliSense for C++**

5. **Installation Location**
   - Default: `C:\Program Files\Microsoft Visual Studio\2022\Community`
   - Installation size: ~8-15 GB

6. **Complete Installation**
   - Click "Install"
   - Wait for installation (20-30 minutes)
   - Restart computer when prompted

#### **Verification:**
```cmd
# Open Command Prompt and run:
where cl
# Should show: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\...
```

---

### **2. CMake (Required)**

#### **Download Link:**
- **Official Website**: https://cmake.org/download/
- **Direct Download**: https://cmake.org/files/v3.28/cmake-3.28.1-windows-x86_64.msi

#### **Installation Steps:**
1. **Download CMake**
   - Go to https://cmake.org/download/
   - Click "Windows x64 Installer"
   - File size: ~50 MB

2. **Run the Installer**
   - Right-click the downloaded `.msi` file
   - Select "Run as administrator"
   - Click "Next" through the setup wizard

3. **Installation Options**
   - ✅ **Add CMake to the system PATH for all users**
   - ✅ **Add CMake to the system PATH for current user**
   - Installation location: `C:\Program Files\CMake`

4. **Complete Installation**
   - Click "Install"
   - Wait for installation (2-3 minutes)
   - Click "Finish"

#### **Verification:**
```cmd
# Open Command Prompt and run:
cmake --version
# Should show: cmake version 3.28.1
```

---

### **3. Windows SDK (Usually Included)**

#### **Check if Already Installed:**
```cmd
# Open Command Prompt and run:
dir "C:\Program Files (x86)\Windows Kits\10\Include"
# Should show version folders like 10.0.22621.0
```

#### **If Not Installed:**
1. **Download Windows SDK**
   - Go to https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
   - Download "Windows 11 SDK"
   - File size: ~1-2 GB

2. **Install Windows SDK**
   - Run the downloaded installer
   - Select "Windows SDK for Windows 11"
   - Complete installation

---

### **4. DirectX 12 (Included with Windows)**

#### **Check DirectX Version:**
```cmd
# Open Command Prompt and run:
dxdiag
# Check "DirectX Version" in the System tab
```

#### **Requirements:**
- **Windows 10** (version 1903 or later) - DirectX 12 included
- **Windows 11** - DirectX 12 included
- **Graphics Card** - DirectX 12 compatible (most modern cards)

---

## **🔍 System Requirements**

### **Minimum Requirements:**
- **OS**: Windows 10 (64-bit) version 1903 or later
- **CPU**: Intel Core i5-8400 / AMD Ryzen 5 2600 or better
- **GPU**: DirectX 12 compatible graphics card
- **RAM**: 8GB system memory
- **Storage**: 20GB free disk space
- **Internet**: Required for downloads

### **Recommended Requirements:**
- **OS**: Windows 11 (64-bit)
- **CPU**: Intel Core i7-10700K / AMD Ryzen 7 3700X or better
- **GPU**: RTX 3070 / RX 6700 XT or better
- **RAM**: 16GB system memory
- **Storage**: 50GB free disk space (SSD recommended)

---

## **📥 Download Links Summary**

| Software | Download Link | File Size | Installation Time |
|----------|---------------|-----------|-------------------|
| **Visual Studio 2022** | https://visualstudio.microsoft.com/downloads/ | ~3-5 GB | 20-30 minutes |
| **CMake** | https://cmake.org/download/ | ~50 MB | 2-3 minutes |
| **Windows SDK** | https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/ | ~1-2 GB | 10-15 minutes |

---

## **✅ Installation Verification**

### **Step 1: Verify Visual Studio**
```cmd
# Open Command Prompt (cmd) and run:
where cl
where link
where lib
# All should show paths to Visual Studio tools
```

### **Step 2: Verify CMake**
```cmd
cmake --version
# Should show: cmake version 3.28.1 or later
```

### **Step 3: Verify Windows SDK**
```cmd
dir "C:\Program Files (x86)\Windows Kits\10\Include"
# Should show version folders
```

### **Step 4: Test Build Environment**
```cmd
# Navigate to your project directory
cd C:\Users\sumit\OneDrive\Desktop\projects\engine

# Create build directory
mkdir build
cd build

# Test CMake configuration
cmake .. -G "Visual Studio 17 2022" -A x64
# Should complete without errors
```

---

## **🚨 Troubleshooting**

### **Problem: "cl is not recognized"**
**Solution:**
1. Open "Developer Command Prompt for VS 2022"
2. Or run: `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`
3. Then try building again

### **Problem: "cmake is not recognized"**
**Solution:**
1. Restart Command Prompt
2. Or add CMake to PATH manually:
   - Add `C:\Program Files\CMake\bin` to system PATH
   - Restart computer

### **Problem: "Windows SDK not found"**
**Solution:**
1. Install Windows SDK from Microsoft website
2. Or install through Visual Studio Installer:
   - Open Visual Studio Installer
   - Modify Visual Studio 2022
   - Add "Windows 11 SDK"

### **Problem: "DirectX 12 not supported"**
**Solution:**
1. Update graphics drivers
2. Check if GPU supports DirectX 12
3. Update Windows to latest version

---

## **🎯 Quick Installation Checklist**

- [ ] **Download Visual Studio 2022 Community**
- [ ] **Install Visual Studio with C++ workload**
- [ ] **Download CMake**
- [ ] **Install CMake with PATH option**
- [ ] **Verify cl compiler works**
- [ ] **Verify cmake works**
- [ ] **Test build environment**
- [ ] **Ready to build AI Movie Studio!**

---

## **📞 Support**

### **If You Need Help:**
1. **Visual Studio Issues**: https://docs.microsoft.com/en-us/visualstudio/
2. **CMake Issues**: https://cmake.org/documentation/
3. **DirectX Issues**: https://docs.microsoft.com/en-us/windows/win32/directx
4. **Windows SDK Issues**: https://docs.microsoft.com/en-us/windows/win32/

### **Common Issues:**
- **Installation takes too long**: Normal, Visual Studio is large
- **Build fails**: Check all prerequisites are installed
- **Permission errors**: Run Command Prompt as Administrator
- **Path issues**: Restart computer after installation

---

**🎬 Once you've installed all prerequisites, you'll be ready to build the AI Movie Studio engine!**

**Next Steps After Installation:**
1. **Verify all tools work**
2. **Run the build script**
3. **Test the engine**
4. **Continue development**

**Let me know when you've completed the installation, and I'll help you build and test the engine!** 🚀✨
