@echo off
REM AI Movie Studio - AI Service Setup Script for Windows

echo ========================================
echo AI Movie Studio - AI Service Setup
echo ========================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python is not installed or not in PATH
    echo Please install Python 3.9+ from https://www.python.org/
    pause
    exit /b 1
)

echo Python found!
echo.

REM Create virtual environment
echo Creating virtual environment...
python -m venv venv
if errorlevel 1 (
    echo ERROR: Failed to create virtual environment
    pause
    exit /b 1
)

echo Virtual environment created!
echo.

REM Activate virtual environment
echo Activating virtual environment...
call venv\Scripts\activate.bat

REM Upgrade pip
echo Upgrading pip...
python -m pip install --upgrade pip

REM Install requirements
echo Installing Python packages...
pip install -r requirements.txt
if errorlevel 1 (
    echo ERROR: Failed to install requirements
    pause
    exit /b 1
)

echo.
echo Installing spaCy language model...
python -m spacy download en_core_web_sm

REM Create directories
echo.
echo Creating directories...
if not exist "logs" mkdir logs
if not exist "models" mkdir models
if not exist "data" mkdir data

REM Copy environment file
if not exist ".env" (
    echo Creating .env file...
    copy .env.example .env
)

echo.
echo ========================================
echo Setup Complete!
echo ========================================
echo.
echo To start the AI service:
echo   1. Activate virtual environment: venv\Scripts\activate
echo   2. Run the service: python main.py
echo.
pause

