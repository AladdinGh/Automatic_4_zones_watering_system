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

## 📡 Network & MQTT Configuration  

Edit these constants in the code:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqttServer = "YOUR_MQTT_BROKER_IP";
const int mqttPort = 1883;
