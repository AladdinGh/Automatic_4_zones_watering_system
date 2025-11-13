##🌿 Automatic_4_zones_watering_system

A system for watering your plants when you are not home.

##🌱 How it works

Moisture sensors in 4 zones deliver their values to the ESP32 board.
Once a zone’s moisture level drops below its threshold, the ESP32 activates the relay for that zone — turning on a small water pump that delivers water through a hose to the plant.

The system connects to Wi-Fi and an MQTT broker, so you can monitor soil moisture and control pumps remotely.
After each watering cycle, the ESP32 goes into deep sleep for one week to save energy.

##⚙️ Parameters

4 irrigation zones (expandable)

Moisture thresholds: 1500 (≈3000 in air, ≈900 in water)

Automatic watering: Each zone irrigates until moisture is above threshold or a timeout occurs

Deep sleep duration: 1 week (604,800 seconds)

Manual control window: 5 minutes after each watering cycle (via MQTT)

Energy optimization: ESP32 sleeps most of the time to minimize power use

##🧠 Features

🌦️ Real-time clock via NTP

💬 MQTT integration for remote monitoring and pump control

🔋 Deep sleep for ultra-low energy consumption

🕒 Configurable sleep duration and manual control time

📡 Publishes sensor data and irrigation status via MQTT

📡 Network & MQTT Configuration

Edit these constants in the code:

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqttServer = "YOUR_MQTT_BROKER_IP";
const int mqttPort = 1883;

MQTT Topics
Topic	Direction	Description
irrigation/status	Publish	System and sleep status messages
irrigation/moisture	Publish	JSON payload with all zone moisture values
irrigation/command	Subscribe	Manual pump control commands
Example Command (to control a pump)

Send this to topic irrigation/command:

{"zone": 2, "action": "on"}


Actions: "on" or "off"
Zones: 1–4

##🧩 Hardware Setup
Zone	Moisture Sensor Pin	Pump Pin
1	GPIO 33	GPIO 13
2	GPIO 32	GPIO 27
3	GPIO 35	GPIO 12
4	GPIO 35 (shared)	GPIO 14

⚠️ Zones 3 and 4 share the same analog pin (GPIO 35). You can change this if you have separate sensors.

##🧰 Required Libraries

Install in Arduino IDE:

WiFi.h (built-in for ESP32)

PubSubClient (by Nick O’Leary)

WiFiUdp.h (built-in)

NTPClient (by Fabrice Weinberg)

##💻 User manual

Modify the paths of Node.js, Mosquitto, and ngrok in RunMe.bat.

Run RunMe.bat.

The web interface should be accessible at:

Local: http://localhost:3000/

Public (via ngrok): e.g. https://elvera-heliographic-corina.ngrok-free.dev/

This web interface allows you to monitor sensor readings and manually control irrigation remotely.

##🔁 Operation Flow

ESP32 wakes up from deep sleep

Connects to Wi-Fi and MQTT

Reads soil moisture values

Publishes data to MQTT

Irrigates dry zones automatically

Publishes updated status

Stays awake for 5 minutes for manual control

Turns all pumps off

Publishes "going to sleep" message

Enters deep sleep for one week

##🧩 Example Serial Output
=== Starting Irrigation Cycle ===
Zone 1 - Value 1820 vs Threshold 1500
DRY → start irrigation
Pump on pin 13 ON
MOIST → stop irrigation
Pump on pin 13 OFF
=== Irrigation Complete ===
Publishing: {"timestamp":"12:30:05","zones":{"zone1":820,"zone2":910,"zone3":870,"zone4":865}}
Manual control window open (5 minutes)...
ESP32 entering deep sleep at 12:35:05

##🔋 Power Saving Notes

Deep sleep current: only a few µA

Use low-power relay modules

Ensure a common ground between all sensors and the ESP32

Recommended: separate power supply for pumps

##🚀 Future Improvements

Web dashboard for live control

Adjustable thresholds via MQTT

Configurable irrigation schedule

Save calibration data in EEPROM