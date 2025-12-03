#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// ============================================================
// =============== Wi-Fi + MQTT CONFIGURATION =================
// ============================================================

const char* ssid = "yourwifi";
const char* password = "yourpw";

const char* mqttServer = "192.168.178.45";
const int mqttPort = 1883;

const char* topicStatus       = "irrigation/status";
const char* topicMoisture     = "irrigation/moisture";
const char* topicCommand      = "irrigation/command";
const char* topicBoardStatus  = "irrigation/boardstatus";   // Dashboard LED topic

WiFiClient espClient;
PubSubClient client(espClient);

// ============================================================
// ======================== NTP TIME ============================
// ============================================================

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

// ============================================================
// ===================== IRRIGATION SETTINGS ====================
// ============================================================

const int number_zones = 4;

// Soil moisture thresholds → higher = more dry
const int moistureThresholds[number_zones] = {1500, 1500, 1500, 1500};

// Relay pins for pumps
const int pumpPins[number_zones] = {13, 27, 12, 14};

// Moisture sensor pins (Zone 3 & 4 share pin 35)
const int moisturePins[number_zones] = {33, 32, 35, 35};

// Max pump ON time per zone (ms)
const unsigned long maxRunTime = 2000;

// Delay between zones (ms)
const unsigned long zoneDelay = 1000;

// Sleep duration: 1 week = 7 days * 24h * 3600s
const unsigned long sleepDurationSec = 604800;

// Manual control window: 5 minutes = 300,000 ms
const int ControlTimeWindow = 300000;


// ============================================================
// ===================== FUNCTION DEFINITIONS ===================
// ============================================================

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
  unsigned long epochTime = timeClient.getEpochTime(); // seconds since 1970

  // convert to date/time
  int days = epochTime / 86400;
  int secondsToday = epochTime % 86400;

  int hour = secondsToday / 3600;
  int minute = (secondsToday % 3600) / 60;
  int second = secondsToday % 60;

  // simple date calculation (year/month/day)
  int year = 1970;
  int dayOfYear = days;
  while (true) {
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    int daysInYear = leap ? 366 : 365;
    if (dayOfYear >= daysInYear) {
      dayOfYear -= daysInYear;
      year++;
    } else break;
  }

  int monthDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if ((year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) monthDays[1] = 29;

  int month = 0;
  while (dayOfYear >= monthDays[month]) {
    dayOfYear -= monthDays[month];
    month++;
  }
  int day = dayOfYear + 1;

  char buf[25];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", year, month+1, day, hour, minute, second);
  return String(buf);
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

  Serial.println("Publishing moisture JSON: " + payload);
  client.publish(topicMoisture, payload.c_str());
}

void irrigateZone(int zoneIndex) {
  unsigned long startTime = millis();
  int moistureValue = readMoisture(zoneIndex);
  int threshold = moistureThresholds[zoneIndex];

  Serial.printf("Zone %d → Moisture %d (Thres %d)\n",
    zoneIndex + 1, moistureValue, threshold);

  if (moistureValue > threshold) {
    Serial.println("DRY → Start irrigation");
    controlPump(pumpPins[zoneIndex], true);

    while (true) {
      client.loop();
      moistureValue = readMoisture(zoneIndex);

      if (moistureValue < threshold) break;
      if (millis() - startTime > maxRunTime) break;

      delay(500);
    }

    controlPump(pumpPins[zoneIndex], false);

  } else {
    Serial.println("MOIST → No irrigation needed");
  }
}


// ============================================================
// ======================== WIFI FIRST =========================
// ============================================================

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected, IP: " + WiFi.localIP().toString());
}


// ============================================================
// ====================== MQTT HANDLING =========================
// ============================================================

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("Received command: " + msg);

  int zoneIndex = -1;
  int zonePos = msg.indexOf("\"zone\"");
  if (zonePos != -1) {
    int colonPos = msg.indexOf(':', zonePos);
    int endPos = msg.indexOf(',', colonPos);
    if (endPos == -1) endPos = msg.indexOf('}', colonPos);
    zoneIndex = msg.substring(colonPos + 1, endPos).toInt() - 1;
  }

  String action;
  int actPos = msg.indexOf("\"action\"");
  if (actPos != -1) {
    int first = msg.indexOf('"', actPos + 8);
    int second = msg.indexOf('"', first + 1);
    action = msg.substring(first + 1, second);
  }

  if (zoneIndex >= 0 && zoneIndex < number_zones) {
    if (action == "on") controlPump(pumpPins[zoneIndex], true);
    else if (action == "off") controlPump(pumpPins[zoneIndex], false);
  }
}

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


// ============================================================
// ==================== IRRIGATION EXECUTION ====================
// ============================================================

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


// ============================================================
// ============================ SETUP ===========================
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  // Set pump pins as outputs
  for (int i = 0; i < number_zones; i++) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH);  // pumps OFF
  }

  connectWiFi();
  timeClient.begin();
  connectMQTT();

  // ============================================================
  // ======== SEND "AWAKE" MESSAGE (for green dashboard LED) =====
  // ============================================================
  Serial.println("Sending AWAKE status to dashboard...");
  client.publish(topicBoardStatus, "{\"status\":\"awake\"}");
  client.loop();
  delay(300);

  runIrrigationCycle();
  publishAllMoisture();

  // ============================================================
  // ========== MANUAL CONTROL WINDOW (5 minutes) ===============
  // ============================================================
  Serial.println("Manual control enabled for 5 minutes...");
  unsigned long start = millis();
  while (millis() - start < ControlTimeWindow) {
    client.loop();
    delay(100);
  }

  // Ensure all pumps are off
  for (int i = 0; i < number_zones; i++) {
    digitalWrite(pumpPins[i], HIGH);
  }

  // ============================================================
  // ============ SEND DEEP SLEEP MESSAGE (RED LED) ==============
  // ============================================================
  String timestamp = getTimestamp();
  String sleepMsg = "{\"status\":\"sleep\",\"time\":\"" + timestamp + "\"}";
  Serial.println("Sending SLEEP status: " + sleepMsg);
  client.publish(topicBoardStatus, sleepMsg.c_str());
  client.loop();
  delay(300);
  // Wifi and MQTT clean shutdown
  client.disconnect();
  WiFi.disconnect(true);
  Serial.flush();

  // ============================================================
  // =================== ENTER DEEP SLEEP (1 WEEK) ===============
  // ============================================================
  esp_sleep_enable_timer_wakeup((uint64_t)sleepDurationSec * 1000000ULL);
  Serial.println("Entering deep sleep now...");
  esp_deep_sleep_start();
}

void loop() {}
