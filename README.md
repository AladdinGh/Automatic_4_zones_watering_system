# 🌿 Automatic_4_zones_watering_system  
A system for watering your plants when you are not home.

---

## 🌱 How it works  

Moisture sensors in **4 zones** deliver their values to the **ESP32** board.  
Once a zone’s moisture level drops below its threshold, the ESP32 activates the **relay** for that zone — turning on a small **water pump** that delivers water through a hose to the plant.  

The system connects to **Wi-Fi** and an **MQTT broker**, so you can monitor soil moisture and control pumps remotely.  
After each watering cycle, the ESP32 goes into **deep sleep for one week** to save energy.

---

## ⚙️ Parameters  

- **4 irrigation zones** (expandable)  
- **Moisture thresholds:** 1500 (≈3000 in air, ≈900 in water)  
- **Automatic watering:** Each zone irrigates until moisture is above threshold or a timeout occurs  
- **Deep sleep duration:** 1 week (604,800 seconds)  
- **Manual control window:** 5 minutes after each watering cycle (via MQTT)  
- **Energy optimization:** ESP32 sleeps most of the time to minimize power use  

---

## 🧠 Features  

- 🌦️ Real-time clock via NTP  
- 💬 MQTT integration for remote monitoring and pump control  
- 🔋 Deep sleep for ultra-low energy consumption  
- 🕒 Configurable sleep duration and manual control time  
- 📡 Publishes sensor data and irrigation status via MQTT  

---

## 🏗️ Build From Scratch

### Local ESP32 + Cloud MQTT + Web Dashboard

This is the most robust and professional setup:

- ESP32 stays on your local network.
- It publishes data (moisture, pump status, board status) to a **cloud MQTT broker**.
- A **web dashboard** subscribes to that MQTT broker to display live data globally.
- No port forwarding or direct exposure of your ESP32 to the Internet is needed.

---

### Steps to Build

1. **ESP32 Code**
   - Connects to local Wi-Fi.
   - Reads moisture sensors.
   - Publishes:
     - Soil moisture (`irrigation/moisture`)
     - Pump status (part of moisture JSON)
     - Board status (`irrigation/boardstatus`) including awake/deep sleep timestamps.
   - Subscribes to `irrigation/command` for manual pump control.

2. **MQTT Broker**
   - Use a **cloud MQTT broker** (e.g., CloudMQTT, HiveMQ) or a local broker.
   - Ensure the ESP32 can connect to the broker.

3. **Web Dashboard (Node.js + Socket.io)**
   - Subscribes to `irrigation/moisture` and `irrigation/boardstatus`.
   - Displays real-time soil moisture and pump status.
   - Displays board state with a virtual LED (green for awake, red for deep sleep).
   - Allows manual pump control via MQTT commands.

---

### Implementation Steps

```bash
# Create project folder
mkdir irrigation-dashboard
cd irrigation-dashboard

# Initialize Node.js project
npm init -y

# Install required packages
npm install express socket.io mqtt
```
1. Create server.js
2. Create public/index.html
3. Start the server : node server.js
4. Access dashboard Local: http://localhost:3000
5. Remote: Use ngrok or host on a cloud server: ngrok http 3000


## 📡 Network & MQTT Configuration  

Edit these constants in the code:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqttServer = "YOUR_MQTT_BROKER_IP";
const int mqttPort = 1883;
```

## 🛰️ MQTT Topics
| Topic                 | Direction | Description                                |
| --------------------- | --------- | ------------------------------------------ |
| `irrigation/status`   | Publish   | System and sleep status messages           |
| `irrigation/moisture` | Publish   | JSON payload with all zone moisture values |
| `irrigation/command`  | Subscribe | Manual pump control commands               |
| `irrigation/boardstatus`  | Publish | ESP32 board state: awake or deep sleep with timestamp               |


⚙️ Example Command (to control a pump)

Send this JSON message to topic irrigation/command:
```Json
{"zone": 2, "action": "on"}
```

zone: The zone number (1–4)

action: "on" to start irrigation or "off" to stop it

💡 Example:
Turn off pump for zone 1:
```Json
{"zone": 1, "action": "off"}
```
## 🧩 Hardware Setup
| Zone | Moisture Sensor Pin | Pump Pin |
| :--: | :-----------------: | :------: |
|   1  |       GPIO 33       |  GPIO 13 |
|   2  |       GPIO 32       |  GPIO 27 |
|   3  |       GPIO 35       |  GPIO 12 |
|   4  |  GPIO 35 *(shared)* |  GPIO 14 |


⚠️ Zones 3 and 4 share the same analog pin (GPIO 35).
You can modify this if you have separate sensors.

## 🧰 Required Libraries

Install the following in the Arduino IDE:

- WiFi.h (built-in for ESP32)

- PubSubClient (by Nick O’Leary)

- WiFiUdp.h (built-in)

- NTPClient (by Fabrice Weinberg)

## 💻 User Manual

1. **Modify configuration**  
   Edit the paths of **Node.js**, **Mosquitto**, and **ngrok** in `RunMe.bat` as needed.

2. **Run the system**  
   Execute `RunMe.bat` to start the application.

3. **Access the web interface**  
   - **Local:** [http://localhost:3000/](http://localhost:3000/)  
   - **Public (via ngrok) 

4. **Features available on the web interface:**  
   - View real-time **soil moisture readings** for all zones  
   - Monitor **irrigation status**  
   - Send **manual pump commands** via MQTT


## 🔁 Operation Flow

1. ESP32 wakes up from deep sleep

2. Connects to Wi-Fi and MQTT broker

3. Reads soil moisture values

4. Publishes readings to MQTT (irrigation/moisture)

5. Automatically irrigates zones if soil is dry

6. Publishes completion message (irrigation/status)

7. Waits 5 minutes for possible manual MQTT control

8. Turns off all pumps

9. Publishes a sleep message and disconnects

10. Enters deep sleep for one week

## 🧩 Example Serial Output

```text
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
```

## 🔋 Power Saving Notes

Deep sleep current: only a few microamps

Use low-power relays or MOSFETs

Ensure a common ground between sensors and the ESP32

Use a separate power supply for pumps (shared ground with ESP32)

🚀 Future Improvements

🌐 Web dashboard for live control

📊 Adjustable thresholds via MQTT

⏰ Configurable irrigation schedule

💾 Save configuration in EEPROM

📱 Integration with mobile app (future version)

## 🧮 Parameter Summary
| Parameter             | Variable            | Default Value | Description                                |
| --------------------- | ------------------- | ------------- | ------------------------------------------ |
| Max run time per zone | `maxRunTime`        | `2000 ms`     | Maximum irrigation duration per zone       |
| Delay between zones   | `zoneDelay`         | `1000 ms`     | Delay between irrigation cycles            |
| Deep sleep duration   | `sleepDurationSec`  | `604800 s`    | Sleep time (1 week)                        |
| Manual control window | `ControlTimeWindow` | `300000 ms`   | Time to control pumps via MQTT (5 minutes) |

## 🧩 Example MQTT Workflow

1. ESP32 publishes soil data:
```Json
{
  "timestamp": "12:00:05",
  "zones": {
    "zone1": 1840,
    "zone2": 1720,
    "zone3": 910,
    "zone4": 850
  }
}
```

2. You send a manual command:
```Json
{"zone": 2, "action": "on"}
```

3. ESP32 responds:

Pump on pin 27 ON


4. When done, ESP32 enters deep sleep and publishes:

ESP32 entering deep sleep at 12:05:05

⚡ Example MQTT Dashboard Setup
| Component   | Example Value                                                    |
| ----------- | ---------------------------------------------------------------- |
| **Broker**  | `mosquitto` on `192.168.178.45`                                  |
| **Port**    | `1883`                                                           |
| **Topics**  | `irrigation/status`, `irrigation/moisture`, `irrigation/command` |
| **UI Tool** | Node-RED / MQTT Explorer / Custom Web Interface                  |

## 🧩 System Overview
[Soil Sensors] → [ESP32 Controller] → [Relays → Pumps]
                        ↓
                 [Wi-Fi Connection]
                        ↓
                 [MQTT Broker/Server]
                        ↓
              [Node.js + Web Dashboard]

## 🧾 License

This project is open-source and available under the MIT License.
Feel free to modify, extend, or integrate it into your own system.

## 👨‍🔧 Author

Developed and maintained by Ala Eddine Gharbi
Based on the concept of a 4-zone automatic irrigation system powered by ESP32 + MQTT.
