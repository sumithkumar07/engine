@echo off
REM AI Movie Studio - Run AI Service

echo ========================================
echo AI Movie Studio - AI Service
echo ========================================
echo.

REM Check if virtual environment exists
if not exist "venv\Scripts\activate.bat" (
    echo ERROR: Virtual environment not found!
    echo Please run setup.bat first
    pause
    exit /b 1
)

REM Activate virtual environment
call venv\Scripts\activate.bat

REM Run the service
echo Starting AI Service...
python main.py

pause

