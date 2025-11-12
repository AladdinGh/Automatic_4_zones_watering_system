#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// === Wi-Fi + MQTT ===
const char* ssid = "FRITZ!Box 7430 OY";
const char* password = "78520207924465391492";
const char* mqttServer = "192.168.178.45";
const int mqttPort = 1883;
const char* topicStatus = "irrigation/status";
const char* topicMoisture = "irrigation/moisture";
const char* topicCommand = "irrigation/command";

WiFiClient espClient;
PubSubClient client(espClient);

// === NTP ===
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC, update every 60s

// === Irrigation Settings ===
const int number_zones = 4;
const int moistureThresholds[number_zones] = {1500, 1500, 1500, 1500};
const int pumpPins[number_zones] = {13, 27, 12, 14};
const int moisturePins[number_zones] = {33, 32, 35, 35};  // Zone 3 & 4 share pin 35
const unsigned long maxRunTime = 2000;       // ms per zone
const unsigned long zoneDelay = 1000;        // ms delay between zones
const unsigned long sleepDurationSec = 10;   // seconds

// === Function to control pump ===
void controlPump(int pumpPin, bool on) {
  digitalWrite(pumpPin, on ? LOW : HIGH);
  Serial.printf("Pump on pin %d %s\n", pumpPin, on ? "ON" : "OFF");
}

// === Read soil moisture ===
int readMoisture(int zoneIndex) {
  if (zoneIndex == 3) return analogRead(moisturePins[2]);
  return analogRead(moisturePins[zoneIndex]);
}

// === Get timestamp ===
String getTimestamp() {
  timeClient.update();
  return timeClient.getFormattedTime(); // HH:MM:SS
}

// === Publish all moisture readings ===
void publishAllMoisture() {
  String timestamp = getTimestamp();
  String payload = "{";
  payload += "\"timestamp\":\"" + timestamp + "\",";
  payload += "\"zones\":{";

  for (int i = 0; i < number_zones; i++) {
    int value = readMoisture(i);
    payload += "\"zone" + String(i+1) + "\":" + String(value);
    if (i < number_zones - 1) payload += ",";
  }
  payload += "}}";

  Serial.println("Publishing: " + payload);
  client.publish(topicMoisture, payload.c_str());
}

// === Automatic irrigation for one zone ===
void irrigateZone(int zoneIndex) {
  unsigned long startTime = millis();
  int moistureValue = readMoisture(zoneIndex);
  int threshold = moistureThresholds[zoneIndex];

  Serial.printf("Zone %d - Value %d vs Threshold %d\n", zoneIndex + 1, moistureValue, threshold);

  if (moistureValue > threshold) {
    Serial.println("DRY → start irrigation");
    controlPump(pumpPins[zoneIndex], true);
    while (true) {
      client.loop(); // handle possible remote pump commands
      moistureValue = readMoisture(zoneIndex);
      if (moistureValue < threshold) {
        Serial.println("MOIST → stop irrigation");
        break;
      }
      if (millis() - startTime > maxRunTime) {
        Serial.println("TIMEOUT → stop irrigation");
        break;
      }
      delay(500);
    }
    controlPump(pumpPins[zoneIndex], false);
  } else {
    Serial.println("MOIST → no irrigation needed");
  }
}

// === Wi-Fi ===
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected, IP: " + WiFi.localIP().toString());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("Received command: " + msg);

  // --- Parse zone ---
  int zoneIndex = -1;
  int zonePos = msg.indexOf("\"zone\"");
  if (zonePos != -1) {
    int colonPos = msg.indexOf(':', zonePos);
    if (colonPos != -1) {
      int endPos = msg.indexOf(',', colonPos);
      if (endPos == -1) endPos = msg.indexOf('}', colonPos);
      String zoneStr = msg.substring(colonPos + 1, endPos);
      zoneStr.trim();
      zoneIndex = zoneStr.toInt() - 1; // convert to 0-based
    }
  }

  // --- Parse action ---
  String action;
  int actPos = msg.indexOf("\"action\"");
  if (actPos != -1) {
    int firstQuote = msg.indexOf('"', actPos + 8);
    int secondQuote = msg.indexOf('"', firstQuote + 1);
    if (firstQuote != -1 && secondQuote != -1) {
      action = msg.substring(firstQuote + 1, secondQuote);
      action.trim();
    }
  }

  Serial.print("Parsed zoneIndex: ");
  Serial.println(zoneIndex);
  Serial.print("Parsed action: ");
  Serial.println(action);

  if (zoneIndex >= 0 && zoneIndex < number_zones) {
    Serial.println("############## Valid command ##############");
    if (action == "on") {
      Serial.println("Action on: ");
      controlPump(pumpPins[zoneIndex], true);
    } else if (action == "off") {
      Serial.println("Action off: ");
      controlPump(pumpPins[zoneIndex], false);
    } else {
      Serial.println("Unknown action value!");
    }
  } else {
    Serial.println("Invalid or out-of-range zoneIndex!");
  }
}


// === MQTT Connection ===
void connectMQTT() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
  Serial.print("Connecting to MQTT");
  while (!client.connected()) {
    if (client.connect("ESP32_Irrigation")) {
      Serial.println("\nMQTT connected");
      client.subscribe(topicCommand);
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

// === Run full irrigation cycle ===
void runIrrigationCycle() {
  Serial.println("=== Starting Irrigation Cycle ===");
  publishAllMoisture();
  for (int i = 0; i < number_zones; i++) {
    irrigateZone(i);
    delay(zoneDelay);
  }
  Serial.println("=== Irrigation Complete ===");
  client.publish(topicStatus, "Irrigation cycle complete");
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  delay(500);

  for (int i = 0; i < number_zones; i++) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH); // pumps off
  }

  connectWiFi();
  timeClient.begin();
  connectMQTT();

  runIrrigationCycle();   // normal automatic watering cycle
  publishAllMoisture();   // final reading

  // Keep ESP32 awake for 10 seconds for possible manual pump control
  Serial.println("Manual control window open (10s)...");
  unsigned long start = millis();
  while (millis() - start < 10000) {
    client.loop(); // allow dashboard control
    delay(100);
  }

  // Turn off all pumps before sleep
  for (int i = 0; i < number_zones; i++) {
    digitalWrite(pumpPins[i], HIGH);
  }

  // Prepare for deep sleep
  String timestamp = getTimestamp();
  String msg = "ESP32 entering deep sleep at " + timestamp;
  Serial.println(msg);
  client.publish(topicStatus, msg.c_str());
  client.loop();

  delay(200);
  client.disconnect();
  WiFi.disconnect(true);
  Serial.flush();

  esp_sleep_enable_timer_wakeup((uint64_t)sleepDurationSec * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}
