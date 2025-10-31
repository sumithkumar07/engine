@echo off
echo Building AI Movie Studio V2.0...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build
cmake --build . --config Release

REM Check if build was successful
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful!
    echo Executable: build\bin\Release\AI_Movie_Studio.exe
    echo.
    echo Running AI Movie Studio V2.0...
    bin\Release\AI_Movie_Studio.exe
) else (
    echo.
    echo Build failed!
    echo Check the output above for errors.
)

pause
