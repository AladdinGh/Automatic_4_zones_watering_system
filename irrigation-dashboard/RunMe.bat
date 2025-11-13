@echo off
echo ============================================
echo Starting Smart Irrigation Control System...
echo ============================================

:: Start Mosquitto MQTT broker
echo [1/3] Starting Mosquitto...
start "" "C:\Program Files\mosquitto\mosquitto.exe" -v
timeout /t 3 /nobreak >nul

:: Start Node.js server
echo [2/3] Starting Node.js server...
cd "C:\Projects\irrigation-dashboard"
start cmd /k "node server.js"
timeout /t 3 /nobreak >nul

:: Start ngrok tunnel
echo [3/3] Starting ngrok tunnel...
cd "C:\ngrok"
start cmd /k "ngrok http 3000"

echo.
echo ============================================
echo All services started!
echo - Mosquitto broker running locally
echo - Node.js dashboard on http://localhost:3000
echo - ngrok tunnel for remote access
echo ============================================
pause
