@echo off
echo Building AI Movie Studio...
cmake --build . --config Debug
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
echo Build successful!
echo Running AI Movie Studio...
.\bin\Debug\AI_Movie_Studio.exe
pause
