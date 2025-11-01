#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// === Wi-Fi + MQTT Settings ===
const char* ssid = "FRITZ!Box 7430 OY";
const char* password = "78520207924465391492";
const char* mqttServer = "192.168.178.45";
const int   mqttPort = 1883;
const char* mqttUser = "";       
const char* mqttPassword = "";  
const char* topicStatus = "irrigation/status";
const char* topicMoisture = "irrigation/moisture";

// === NTP Settings ===
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC, update every 60s

WiFiClient espClient;
PubSubClient client(espClient);

// === Irrigation Settings ===
const int number_zones = 4;
const int moistureThresholds[number_zones] = {1500, 1500, 1500, 1500};
const int pumpPins[number_zones] = {13, 27, 12, 14};
const int moisturePins[number_zones] = {33, 32, 35, 35};  // Zone 3 & 4 share pin 35
const unsigned long maxRunTime = 2000;       // ms
const unsigned long zoneDelay = 1000;        // ms
const unsigned long sleepDurationSec = 10;   // 10 seconds

// === Functions ===
void controlPump(int pumpPin, bool on) {
  digitalWrite(pumpPin, on ? LOW : HIGH);
  Serial.printf("Pump on pin %d %s\n", pumpPin, on ? "ON" : "OFF");
}

int readMoisture(int zoneIndex) {
  if (zoneIndex == 3) return analogRead(moisturePins[2]);
  return analogRead(moisturePins[zoneIndex]);
}

String getTimestamp() {
  timeClient.update();
  return timeClient.getFormattedTime(); // HH:MM:SS
}

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

void irrigateZone(int zoneIndex) {
  unsigned long startTime = millis();
  int moistureValue = readMoisture(zoneIndex);
  int threshold = moistureThresholds[zoneIndex];

  Serial.printf("Zone %d - Value %d vs Threshold %d\n", zoneIndex + 1, moistureValue, threshold);

  if (moistureValue > threshold) {
    Serial.println("DRY → start irrigation");
    controlPump(pumpPins[zoneIndex], true);
    while (true) {
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

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected, IP: " + WiFi.localIP().toString());
}

void connectMQTT() {
  client.setServer(mqttServer, mqttPort);
  Serial.print("Connecting to MQTT");
  while (!client.connected()) {
    if (client.connect("ESP32_Irrigation", mqttUser, mqttPassword)) {
      Serial.println("\nMQTT connected");
    } else {
      Serial.print(".");
      delay(1000);
    }
  }
}

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
    digitalWrite(pumpPins[i], HIGH);  // pumps off
  }

  connectWiFi();
  timeClient.begin();
  connectMQTT();

  runIrrigationCycle();
  publishAllMoisture();

  // === Sleep Preparation ===
  String timestamp = getTimestamp();
  String msg = "ESP32 entering deep sleep at " + timestamp;
  Serial.println(msg);
  client.publish(topicStatus, msg.c_str());
  client.loop(); // ensure message is sent before disconnect

  delay(200); // small delay to flush publish
  client.disconnect();
  WiFi.disconnect(true);

  Serial.flush();
  esp_sleep_enable_timer_wakeup((uint64_t)sleepDurationSec * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}
