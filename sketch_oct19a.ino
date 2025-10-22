// === Multi-Zone Irrigation System ===
// Each zone: 1 pump relay + 1 moisture sensor

const int pumpPins[4] = {5, 18, 19, 21};       // Relay output pins
const int moisturePins[4] = {15, 4, 2, 22};    // Moisture sensor input pins

// === Threshold and Timing Settings ===
const int moistureThreshold = 1500;  // Adjust based on your soil
const unsigned long checkInterval = 1000;  // Check every 2s while watering
const unsigned long maxRunTime = 60000;    // 60s safety cutoff per zone
const unsigned long irrigationInterval = 1000; // 1 min for testing (24h later)

// === Timing Variables ===
unsigned long lastIrrigationTime = 0;

void setup() {
  Serial.begin(115200);

  // Initialize pumps and sensors
  for (int i = 0; i < 4; i++) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH);  // Ensure pumps OFF (active LOW)
    pinMode(moisturePins[i], INPUT_PULLUP);
  }

  Serial.println("=== 4-Zone Irrigation System Initialized ===");
  Serial.println("Dry ≈ 3000 | Wet ≈ 980 | Threshold ≈ 1500");
}

// === Read Moisture Sensor ===
int readMoisture(int sensorPin) {
  int value = analogRead(sensorPin);
  Serial.print("Moisture Reading (Pin ");
  Serial.print(sensorPin);
  Serial.print("): ");
  Serial.println(value);
  return value;
}

// === Control Pump (active LOW relay) ===
void controlPump(int pumpPin, bool on) {
  digitalWrite(pumpPin, on ? LOW : HIGH);  // LOW = ON for relay
  Serial.print("Pump on pin ");
  Serial.print(pumpPin);
  Serial.println(on ? " ON" : " OFF");
}

// === Irrigation Logic for a Single Zone ===
void irrigateZone(int zoneIndex) {
  unsigned long startTime = millis();

  int moistureValue = readMoisture(moisturePins[zoneIndex]);

  // Check if soil is dry
  if (moistureValue > moistureThreshold) {
    Serial.print("Zone ");
    Serial.print(zoneIndex + 1);
    Serial.println(" is DRY — starting irrigation...");
    controlPump(pumpPins[zoneIndex], true);

    // Keep watering until moist or timeout
    while (true) {
      delay(checkInterval);
      moistureValue = readMoisture(moisturePins[zoneIndex]);

      if (moistureValue < moistureThreshold) {
        Serial.print("Zone ");
        Serial.print(zoneIndex + 1);
        Serial.println(" is now MOIST — stopping irrigation.");
        break;
      }

      if (millis() - startTime > maxRunTime) {
        Serial.print("Zone ");
        Serial.print(zoneIndex + 1);
        Serial.println(" timeout — stopping pump for safety.");
        break;
      }
    }

    controlPump(pumpPins[zoneIndex], false);
  } else {
    Serial.print("Zone ");
    Serial.print(zoneIndex + 1);
    Serial.println(" is already moist — no irrigation needed.");
  }
}

// === Run All Zones ===
void runIrrigationCycle() {
  Serial.println("=== Starting Irrigation Cycle for All Zones ===");
  for (int i = 0; i < 4; i++) {
    irrigateZone(i);
    //delay(1000); // Small delay between zones
  }
  Serial.println("=== Irrigation Cycle Complete ===");
}

// === Main Loop ===
void loop() {
  unsigned long currentTime = millis();

  // Run once per interval (set to 1 min for testing)
  if (currentTime - lastIrrigationTime >= irrigationInterval) {
    runIrrigationCycle();
    lastIrrigationTime = currentTime;
  }

  delay(1000);
}
